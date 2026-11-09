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


float encoder_get_rpm_real(uint32_t cpr) {
    static uint32_t last_time = 0;
    static int32_t last_count = 0;
    static float last_calculated_rpm = 0.0f;
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed_time = current_time - last_time;

    // Przeliczamy obroty, gdy minie chociaż 20 ms, żeby nie obciążać procesora w każdej mikrosekundzie
    if (elapsed_time >= 20) {
        int32_t current_count = encoder_get_count();
        int32_t delta_ticks = current_count - last_count;

        last_count = current_count;
        last_time = current_time;

        // Wzór: (zmiana impulsów / CPR) * (60000 ms / czas w ms)
        float rps = ((float)delta_ticks / (float)cpr) * (1000.0f / (float)elapsed_time);
        last_calculated_rpm = rps*60.0f;
    }

    return last_calculated_rpm;
}



