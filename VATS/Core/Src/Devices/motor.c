/*
 * motor.c
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */

#include "Devices/motor.h"
#include "Devices/enkoder.h"
#define AIN1_GPIO_PORT GPIOC
#define AIN1_PIN GPIO_PIN_7
#define AIN2_GPIO_PORT GPIOC
#define AIN2_PIN GPIO_PIN_9

void motor_init(void){
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
	HAL_GPIO_WritePin(AIN1_GPIO_PORT, AIN1_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(AIN2_GPIO_PORT, AIN2_PIN, GPIO_PIN_RESET);
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
    static GPIO_PinState last_button_state = GPIO_PIN_SET;


    GPIO_PinState current_button_state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

    if (current_button_state == GPIO_PIN_RESET && last_button_state == GPIO_PIN_SET) {
        speed += 200;

        if (speed > 1000) {
            speed = 0;
        }

        motor_set_speed(speed);


        HAL_Delay(200);
    }

    last_button_state = current_button_state;
}
