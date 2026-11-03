/*
 * enkoder.h
 *
 *  Created on: Oct 19, 2026
 *      Author: bartol
 */
#include "main.h"
#include "tim.h"
#include <stdio.h>
#ifndef INC_DEVICES_ENKODER_H_
#define INC_DEVICES_ENKODER_H_

void encoder_init(void);

int32_t encoder_get_count(void);

void encoder_reset(void);


float encoder_get_rpm_real(uint32_t cpr);

#endif /* INC_DEVICES_ENKODER_H_ */
