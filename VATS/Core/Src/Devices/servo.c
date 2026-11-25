/*
 * servo.c
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */

#include "Devices/servo.h"
#include "tim.h"
#include <stdbool.h>

#define SERVO_TIM            (&htim4)
#define SERVO_CHANNEL        TIM_CHANNEL_3
#define SERVO_TICK_HZ        1000000U   /* wymagane: 1 tick = 1 us */
#define SERVO_PERIOD_TICKS   20000U     /* 20 ms = 50 Hz */

static bool    servo_ready = false;
static int32_t current_angle = SERVO_ANGLE_CENTER;

static uint32_t angle_to_pulse_us(int32_t angle_deg)
{
    int32_t span = SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US;
    int32_t pulse = SERVO_PULSE_MIN_US + (angle_deg * span) / 180 + SERVO_CENTER_TRIM_US;
    if (pulse < SERVO_PULSE_MIN_US) pulse = SERVO_PULSE_MIN_US;
    if (pulse > SERVO_PULSE_MAX_US) pulse = SERVO_PULSE_MAX_US;
    return (uint32_t)pulse;
}

HAL_StatusTypeDef servo_init(void)
{
    /* Sprawdzenie, że konfiguracja TIM4 z CubeMX daje 1 MHz i 50 Hz.
     * Zakłada APB1 prescaler = 1 (wtedy zegar timera = PCLK1). */
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    if (pclk1 != HAL_RCC_GetHCLKFreq()) {
        return HAL_ERROR;   /* APB1 != HCLK: zegar timera = 2 x PCLK1, przelicz PSC */
    }
    uint32_t tick_hz = pclk1 / (SERVO_TIM->Init.Prescaler + 1U);
    uint32_t period  = __HAL_TIM_GET_AUTORELOAD(SERVO_TIM) + 1U;
    if (tick_hz != SERVO_TICK_HZ || period != SERVO_PERIOD_TICKS) {
        return HAL_ERROR;
    }

    current_angle = SERVO_ANGLE_CENTER;
    __HAL_TIM_SET_COMPARE(SERVO_TIM, SERVO_CHANNEL, angle_to_pulse_us(current_angle));

    HAL_StatusTypeDef status = HAL_TIM_PWM_Start(SERVO_TIM, SERVO_CHANNEL);
    if (status != HAL_OK) {
        return status;
    }
    servo_ready = true;
    return HAL_OK;
}

HAL_StatusTypeDef servo_set_angle(int32_t angle_deg)
{
    if (!servo_ready) {
        return HAL_ERROR;
    }
    if (angle_deg < SERVO_ANGLE_MIN) angle_deg = SERVO_ANGLE_MIN;
    if (angle_deg > SERVO_ANGLE_MAX) angle_deg = SERVO_ANGLE_MAX;

    current_angle = angle_deg;
    __HAL_TIM_SET_COMPARE(SERVO_TIM, SERVO_CHANNEL, angle_to_pulse_us(angle_deg));
    return HAL_OK;
}

int32_t servo_get_angle(void)
{
    return current_angle;
}
