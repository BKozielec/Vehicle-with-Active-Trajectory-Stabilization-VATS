/*
 * enkoder.c
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */
#include "Devices/enkoder.h"




void encoder_init(void){
HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);
}

int32_t encoder_get_count(void){
return __HAL_TIM_GET_COUNTER(&htim3);
}

void encoder_reset(void){
	__HAL_TIM_SET_COUNTER(&htim3,0);
}
