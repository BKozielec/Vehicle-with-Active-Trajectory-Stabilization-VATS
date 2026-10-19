/*
 * testdiode.h
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */

#include "Devices/testdiode.h"

extern TIM_HandleTypeDef htim1;

void TestDiode_Init(void){
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
}

void TestDiode_SetBrightness(uint32_t duty_cycle){
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1,duty_cycle);
}
