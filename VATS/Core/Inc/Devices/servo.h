/*
 * servo.h
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 *
 * Serwo SG90: PB8 = TIM4_CH3, 50 Hz (PSC 31, ARR 19999 przy 32 MHz -> 1 tick = 1 us).
 */

#ifndef INC_DEVICES_SERVO_H_
#define INC_DEVICES_SERVO_H_

#include "main.h"
#include "vats_config.h"

HAL_StatusTypeDef servo_init(void);

/* Kąt 0..180 (90 = prosto); przycinany do SERVO_ANGLE_MIN..SERVO_ANGLE_MAX. */
HAL_StatusTypeDef servo_set_angle(int32_t angle_deg);

int32_t servo_get_angle(void);

#endif /* INC_DEVICES_SERVO_H_ */
