/*
 * motor.c
 *
 * Created on: Oct 19, 2026
 * Author: bartol
 */

#include "Devices/motor.h"
#include "Devices/esp.h"
#include "Devices/enkoder.h"
#include <math.h>


#define MOTOR_DIR1_PORT GPIOC
#define MOTOR_DIR1_PIN GPIO_PIN_11
#define MOTOR_DIR2_PORT GPIOC
#define MOTOR_DIR2_PIN GPIO_PIN_12

extern TIM_HandleTypeDef htim1;

void motor_init(void){

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);


    motor_set_direction('S');
    motor_set_speed(0);
}


void motor_set_speed(uint32_t speed) {
    if (speed > 1000) speed = 1000;
    if (speed>0 && speed<100) speed=100;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, speed);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, speed);
}


void motor_set_direction(char dir) {
    if (dir == 'F') {
        HAL_GPIO_WritePin(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_DIR2_PORT, MOTOR_DIR2_PIN, GPIO_PIN_RESET);
    } else if (dir == 'B') {
        HAL_GPIO_WritePin(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_DIR2_PORT, MOTOR_DIR2_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(MOTOR_DIR1_PORT, MOTOR_DIR1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_DIR2_PORT, MOTOR_DIR2_PIN, GPIO_PIN_RESET);
    }
}


void motor_update_from_esp(void) {
    int target_speed = ESP_GetSpeed();
    char target_dir = ESP_GetDirection();

    float current_rpm = encoder_get_rpm_real(2000);
    float current_rpm_abs = fabsf(current_rpm);
    float max_motor_rpm = 104.0f;
    float current_pwm_equivalent = (current_rpm_abs / max_motor_rpm) * 1000.0f;
    if (current_pwm_equivalent > 1000.0f) current_pwm_equivalent = 1000.0f;
    float pid_output = PID_compute((float)target_speed, current_pwm_equivalent);

    motor_set_speed((uint32_t)pid_output);
    motor_set_direction(target_dir);
}
