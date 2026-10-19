/*
 * motor.c
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */

#include "Devices/motor.h"
#include "Devices/enkoder.h"

void motor_init(void){
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
}

void motor_set_speed(uint32_t speed){
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,speed);
}


uint32_t motor_read_speed(void){
	GPIO_PinState state = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
	return (uint32_t) state;
}

void motor_test(void) {

    static uint32_t speed = 0;

    if (motor_read_speed() == GPIO_PIN_SET) {
        speed += 100;


        if (speed > 1000) {
            speed = 0;
        }

        motor_set_speed(speed);
    }
}

//void motor_update(void){
//	int32_t current_pulse=encoder_get_count();
//	static uint32_t speed_counter = 0;
//}

