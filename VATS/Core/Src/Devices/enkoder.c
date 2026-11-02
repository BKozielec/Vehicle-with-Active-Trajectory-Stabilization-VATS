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

float encoder_get_rpm(uint32_t cpr, uint32_t interval_ms){
	static int32_t last_count =0;
	int32_t current_count = encoder_get_count();
	int32_t delta_ticks=current_count-last_count;
	last_count=current_count;
	float rps=((float)delta_ticks/(float)cpr)*(1000.0f / (float)interval_ms);
	return rps * 60.0f;
}





