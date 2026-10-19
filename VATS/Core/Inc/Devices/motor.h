/*
 * motor.h
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */
#include "main.h"
#include "tim.h"

#ifndef INC_DEVICES_MOTOR_H_
#define INC_DEVICES_MOTOR_H_

void motor_init(void);

uint32_t motor_read_speed(void);

void motor_set_speed(uint32_t speed);
void motor_update(void);
void motor_test(void);



#endif /* INC_DEVICES_MOTOR_H_ */
