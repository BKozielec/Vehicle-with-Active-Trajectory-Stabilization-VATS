/*
 * pid.h
 *
 *  Created on: Nov 3, 2026
 *      Author: bartol
 *
 * Dyskretny regulator PID z jawnym dt, feedforwardem, saturacją wyjścia,
 * anti-windupem (integracja warunkowa) i członem D liczonym z pomiaru.
 */

#ifndef INC_PID_H_
#define INC_PID_H_

#include <stdbool.h>

typedef struct {
    float kp;               /* [wyjście / jednostka uchybu] */
    float ki;               /* [wyjście / (jednostka uchybu * s)] */
    float kd;               /* [wyjście * s / jednostka uchybu] */
    float out_min;
    float out_max;
    float integral;         /* przechowywane w jednostkach wyjścia */
    float prev_measurement;
    bool  has_prev;
} PID_Controller;

void  PID_Init(PID_Controller *pid, float kp, float ki, float kd,
               float out_min, float out_max);
void  PID_Reset(PID_Controller *pid);
float PID_Compute(PID_Controller *pid, float setpoint, float measurement,
                  float feedforward, float dt_s);

#endif /* INC_PID_H_ */
