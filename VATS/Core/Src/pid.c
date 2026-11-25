/*
 * pid.c
 *
 *  Created on: Nov 3, 2026
 *      Author: bartol
 */

#include "pid.h"
#include <stddef.h>

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void PID_Init(PID_Controller *pid, float kp, float ki, float kd,
              float out_min, float out_max)
{
    if (pid == NULL) return;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    PID_Reset(pid);
}

void PID_Reset(PID_Controller *pid)
{
    if (pid == NULL) return;
    pid->integral = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->has_prev = false;
}

float PID_Compute(PID_Controller *pid, float setpoint, float measurement,
                  float feedforward, float dt_s)
{
    if (pid == NULL) return 0.0f;
    if (dt_s <= 0.0f) {
        return clampf(feedforward, pid->out_min, pid->out_max);
    }

    float error = setpoint - measurement;

    /* D z pomiaru – brak "derivative kick" przy skoku setpointu */
    float derivative = 0.0f;
    if (pid->has_prev) {
        derivative = -pid->kd * (measurement - pid->prev_measurement) / dt_s;
    }
    pid->prev_measurement = measurement;
    pid->has_prev = true;

    float p_term = pid->kp * error;
    float integral_candidate = pid->integral + pid->ki * error * dt_s;
    float output = feedforward + p_term + integral_candidate + derivative;

    /* Anti-windup: nie całkujemy, gdy wyjście jest w nasyceniu,
     * a uchyb pcha je jeszcze dalej w to nasycenie. */
    bool saturated_high = (output > pid->out_max) && (error > 0.0f);
    bool saturated_low  = (output < pid->out_min) && (error < 0.0f);
    if (!saturated_high && !saturated_low) {
        float range = pid->out_max - pid->out_min;
        pid->integral = clampf(integral_candidate, -range, range);
    }

    output = feedforward + p_term + pid->integral + derivative;
    return clampf(output, pid->out_min, pid->out_max);
}
