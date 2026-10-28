/*
 * motor.h
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */
#include "main.h"
#include "tim.h"

#ifndef DEVICES_MOTOR_H_
#define DEVICES_MOTOR_H_

void motor_init(void);
void motor_set_speed(uint32_t speed);
void motor_set_direction(char dir);
void motor_update_from_esp(void);

#endif /* DEVICES_MOTOR_H_ */
