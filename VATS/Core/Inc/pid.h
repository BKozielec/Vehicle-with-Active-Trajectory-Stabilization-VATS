/*
 * pid.h
 *
 *  Created on: Nov 3, 2026
 *      Author: bartol
 */

#ifndef INC_PID_H_
#define INC_PID_H_

typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_error;
} PID_Controller;

void PID_Init(void);

float PID_compute(float setpoint, float measure);




#endif /* INC_PID_H_ */
