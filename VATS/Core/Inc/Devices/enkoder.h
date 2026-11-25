/*
 * enkoder.h
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 *
 * Dwa enkodery kwadraturowe: lewy TIM3 (PA6/PA7), prawy TIM8 (PB10/PB13).
 */

#ifndef INC_DEVICES_ENKODER_H_
#define INC_DEVICES_ENKODER_H_

#include "main.h"
#include "vats_config.h"

HAL_StatusTypeDef encoder_init(void);

/* Wywoływać raz na okres regulacji; dt_ms = czas od poprzedniego wywołania. */
void    encoder_update(uint32_t dt_ms);

/* Prędkość koła ze znakiem [RPM], wartość z ostatniego encoder_update(). */
float   encoder_get_rpm(Wheel wheel);

/* Skumulowana pozycja w impulsach licznika (do pomiaru CPR). */
int32_t encoder_get_position(Wheel wheel);

#endif /* INC_DEVICES_ENKODER_H_ */
