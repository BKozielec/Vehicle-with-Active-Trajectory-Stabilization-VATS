/*
 * motor.c
 *
 * Created on: Oct 19, 2026
 * Author: bartol
 */

#include "Devices/motor.h"
#include "Devices/enkoder.h"
#include "pid.h"
#include "tim.h"
#include <math.h>
#include <stdbool.h>

#define AIN1_PORT  GPIOC
#define AIN1_PIN   GPIO_PIN_7
#define AIN2_PORT  GPIOC
#define AIN2_PIN   GPIO_PIN_9
#define BIN1_PORT  GPIOC
#define BIN1_PIN   GPIO_PIN_11
#define BIN2_PORT  GPIOC
#define BIN2_PIN   GPIO_PIN_12

typedef enum {
    MOTOR_MODE_CLOSED_LOOP = 0,
    MOTOR_MODE_OPEN_LOOP
} MotorMode;

typedef struct {
    uint32_t       tim_channel;
    GPIO_TypeDef  *in1_port;
    uint16_t       in1_pin;
    GPIO_TypeDef  *in2_port;
    uint16_t       in2_pin;
    bool           invert;
    char           pin_dir;   /* kierunek aktualnie ustawiony na pinach */
    uint32_t       duty;
    PID_Controller pid;
} MotorChannel;

static MotorChannel motors[WHEEL_COUNT] = {
    [WHEEL_LEFT]  = { TIM_CHANNEL_1, AIN1_PORT, AIN1_PIN, AIN2_PORT, AIN2_PIN,
                      (MOTOR_L_INVERT != 0), 'S', 0U, {0} },
    [WHEEL_RIGHT] = { TIM_CHANNEL_2, BIN1_PORT, BIN1_PIN, BIN2_PORT, BIN2_PIN,
                      (MOTOR_R_INVERT != 0), 'S', 0U, {0} },
};

static MotorMode mode           = MOTOR_MODE_CLOSED_LOOP;
static char      requested_dir  = 'S';
static char      applied_dir    = 'S';
static float     target_rpm     = 0.0f;
static uint32_t  open_loop_duty = 0U;

static bool is_valid_dir(char dir)
{
    return (dir == 'F') || (dir == 'B') || (dir == 'S');
}

static void reset_pids(void)
{
    for (int i = 0; i < WHEEL_COUNT; i++) {
        PID_Reset(&motors[i].pid);
    }
}

/* Ustawia piny kierunku i duty jednego kanału. dir 'S' = wybieg (IN1=IN2=L). */
static void motor_apply(MotorChannel *m, char dir, uint32_t duty)
{
    if (duty > MOTOR_DUTY_MAX) duty = MOTOR_DUTY_MAX;

    char pin_dir = dir;
    if (m->invert) {
        if (pin_dir == 'F')      pin_dir = 'B';
        else if (pin_dir == 'B') pin_dir = 'F';
    }
    if (pin_dir == 'S') {
        duty = 0U;
    }

    if (pin_dir != m->pin_dir) {
        /* Najpierw zdejmij PWM, potem zmień kierunek. */
        __HAL_TIM_SET_COMPARE(&htim1, m->tim_channel, 0U);
        if (pin_dir == 'F') {
            HAL_GPIO_WritePin(m->in2_port, m->in2_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(m->in1_port, m->in1_pin, GPIO_PIN_SET);
        } else if (pin_dir == 'B') {
            HAL_GPIO_WritePin(m->in1_port, m->in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(m->in2_port, m->in2_pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(m->in1_port, m->in1_pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(m->in2_port, m->in2_pin, GPIO_PIN_RESET);
        }
        m->pin_dir = pin_dir;
    }

    __HAL_TIM_SET_COMPARE(&htim1, m->tim_channel, duty);
    m->duty = duty;
}

HAL_StatusTypeDef motor_init(void)
{
    for (int i = 0; i < WHEEL_COUNT; i++) {
        MotorChannel *m = &motors[i];
        PID_Init(&m->pid, SPEED_KP, SPEED_KI, SPEED_KD,
                 0.0f, (float)MOTOR_DUTY_MAX);
        m->pin_dir = '?';            /* wymuś zapis pinów w motor_apply */
        motor_apply(m, 'S', 0U);
    }

    for (int i = 0; i < WHEEL_COUNT; i++) {
        HAL_StatusTypeDef status = HAL_TIM_PWM_Start(&htim1, motors[i].tim_channel);
        if (status != HAL_OK) {
            motor_emergency_stop();
            return status;
        }
    }

    mode = MOTOR_MODE_CLOSED_LOOP;
    requested_dir = 'S';
    applied_dir = 'S';
    target_rpm = 0.0f;
    open_loop_duty = 0U;
    return HAL_OK;
}

void motor_set_target(char dir, uint32_t speed_rpm)
{
    if (!is_valid_dir(dir)) return;

    float rpm = (float)speed_rpm;
    if (rpm > MOTOR_MAX_RPM) rpm = MOTOR_MAX_RPM;

    if (mode != MOTOR_MODE_CLOSED_LOOP) {
        reset_pids();
    }
    mode = MOTOR_MODE_CLOSED_LOOP;
    requested_dir = dir;
    target_rpm = rpm;
}

void motor_set_open_loop(char dir, uint32_t duty)
{
    if (!is_valid_dir(dir)) return;
    if (duty > MOTOR_DUTY_MAX) duty = MOTOR_DUTY_MAX;

    mode = MOTOR_MODE_OPEN_LOOP;
    requested_dir = dir;
    open_loop_duty = duty;
    reset_pids();
}

void motor_stop(void)
{
    requested_dir = 'S';
    applied_dir = 'S';
    target_rpm = 0.0f;
    open_loop_duty = 0U;
    reset_pids();
    for (int i = 0; i < WHEEL_COUNT; i++) {
        motor_apply(&motors[i], 'S', 0U);
    }
}

void motor_control_update(uint32_t dt_ms)
{
    float dt_s = (float)dt_ms / 1000.0f;

    float max_abs_rpm = 0.0f;
    for (int i = 0; i < WHEEL_COUNT; i++) {
        float r = fabsf(encoder_get_rpm((Wheel)i));
        if (r > max_abs_rpm) max_abs_rpm = r;
    }

    /* Zmiana kierunku zawsze przez wybieg 'S' aż do prawie zatrzymania. */
    if (requested_dir == 'S') {
        applied_dir = 'S';
    } else if (applied_dir == 'S') {
        if (max_abs_rpm < MOTOR_DIR_CHANGE_RPM) {
            applied_dir = requested_dir;
            reset_pids();
        }
    } else if (applied_dir != requested_dir) {
        applied_dir = 'S';
        reset_pids();
    }

    for (int i = 0; i < WHEEL_COUNT; i++) {
        MotorChannel *m = &motors[i];

        if (applied_dir == 'S') {
            motor_apply(m, 'S', 0U);
            continue;
        }

        if (mode == MOTOR_MODE_OPEN_LOOP) {
            motor_apply(m, applied_dir, open_loop_duty);
            continue;
        }

        if (target_rpm <= 0.0f) {
            PID_Reset(&m->pid);
            motor_apply(m, applied_dir, 0U);
            continue;
        }

        /* Regulacja na wartości bezwzględnej – kierunek wynika z pinów,
         * więc znak enkodera nie wpływa na stabilność pętli. */
        float measurement = fabsf(encoder_get_rpm((Wheel)i));
        float feedforward = (float)MOTOR_DUTY_MIN
                          + (target_rpm / MOTOR_MAX_RPM)
                          * (float)(MOTOR_DUTY_MAX - MOTOR_DUTY_MIN);
        float u = PID_Compute(&m->pid, target_rpm, measurement, feedforward, dt_s);
        motor_apply(m, applied_dir, (uint32_t)(u + 0.5f));
    }
}

int32_t motor_get_duty(Wheel wheel)
{
    if ((unsigned)wheel >= (unsigned)WHEEL_COUNT) return 0;
    int32_t duty = (int32_t)motors[wheel].duty;
    return (applied_dir == 'B') ? -duty : duty;
}

void motor_emergency_stop(void)
{
    /* Bez HAL i bez przerwań: PWM = 0, wszystkie IN = L (wybieg). */
    TIM1->CCR1 = 0U;
    TIM1->CCR2 = 0U;
    for (int i = 0; i < WHEEL_COUNT; i++) {
        motors[i].in1_port->BSRR = (uint32_t)motors[i].in1_pin << 16U;
        motors[i].in2_port->BSRR = (uint32_t)motors[i].in2_pin << 16U;
    }
}
