/*
 * enkoder.c
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */

#include "Devices/enkoder.h"
#include "tim.h"

typedef struct {
    TIM_HandleTypeDef *htim;
    int8_t   sign;
    uint16_t last_raw;
    int32_t  position;
    float    rpm;
} Encoder;

static Encoder encoders[WHEEL_COUNT] = {
    [WHEEL_LEFT]  = { &htim3, (ENCODER_L_INVERT ? -1 : 1), 0U, 0, 0.0f },
    [WHEEL_RIGHT] = { &htim8, (ENCODER_R_INVERT ? -1 : 1), 0U, 0, 0.0f },
};

HAL_StatusTypeDef encoder_init(void)
{
    for (int i = 0; i < WHEEL_COUNT; i++) {
        Encoder *e = &encoders[i];
        HAL_StatusTypeDef status = HAL_TIM_Encoder_Start(e->htim, TIM_CHANNEL_ALL);
        if (status != HAL_OK) {
            return status;
        }
        e->last_raw = (uint16_t)__HAL_TIM_GET_COUNTER(e->htim);
        e->position = 0;
        e->rpm = 0.0f;
    }
    return HAL_OK;
}

void encoder_update(uint32_t dt_ms)
{
    if (dt_ms == 0U) {
        return;
    }
    for (int i = 0; i < WHEEL_COUNT; i++) {
        Encoder *e = &encoders[i];
        uint16_t raw = (uint16_t)__HAL_TIM_GET_COUNTER(e->htim);
        /* Różnica modulo 2^16 rzutowana na int16_t poprawnie obsługuje
         * przepełnienie licznika w obu kierunkach, o ile w jednym okresie
         * przybędzie mniej niż 32768 impulsów. */
        int16_t delta = (int16_t)(uint16_t)(raw - e->last_raw);
        e->last_raw = raw;

        int32_t signed_delta = (int32_t)delta * (int32_t)e->sign;
        e->position += signed_delta;
        e->rpm = ((float)signed_delta / ENCODER_CPR) * (60000.0f / (float)dt_ms);
    }
}

float encoder_get_rpm(Wheel wheel)
{
    if ((unsigned)wheel >= (unsigned)WHEEL_COUNT) return 0.0f;
    return encoders[wheel].rpm;
}

int32_t encoder_get_position(Wheel wheel)
{
    if ((unsigned)wheel >= (unsigned)WHEEL_COUNT) return 0;
    return encoders[wheel].position;
}
