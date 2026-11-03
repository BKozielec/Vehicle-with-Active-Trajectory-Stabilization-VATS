/*
 * pid.c
 *
 *  Created on: Nov 3, 2026
 *      Author: bartol
 */

#include "pid.h"

PID_Controller motor_pid;

void PID_Init(void){
motor_pid.kp = 1.2f; //Wzmocnienie proporcjonalne
motor_pid.ki = 0.2f; //Wzmocnienie całkujące
motor_pid.kd = 0.05f;// Wzmocnienie różniczkujące
motor_pid.integral=0.0f; //
motor_pid.prev_error=0.0f; //
}

float PID_compute(float setpoint, float measure){
	float error = setpoint - measure;
	motor_pid.integral +=error;
	if(motor_pid.integral>1000.0f) motor_pid.integral= 1000.0f;
	if(motor_pid.integral<-1000.0f) motor_pid.integral=-1000.0f;

	float derivative=error - motor_pid.prev_error;
	float output = (motor_pid.kp * error) + (motor_pid.ki * motor_pid.integral) + (motor_pid.kd * derivative);
	motor_pid.prev_error=error;
	return output;
}


