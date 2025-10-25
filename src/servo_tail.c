#include "servo_tail.h"
#include <math.h>

static float clampf(float v, float a, float b){ return v<a?a : (v>b?b : v); }
static uint16_t deg_to_us(const ServoTail *st, float deg){
    const float span_us = (float)(st->us_max - st->us_min);
    float norm = (deg + 90.0f) / 180.0f; // [-90..+90] -> [0..1]
    float us = (float)st->us_min + norm * span_us;
    if(us < st->us_min) us = (float)st->us_min;
    if(us > st->us_max) us = (float)st->us_max;
    return (uint16_t)us;
}

void st_init(ServoTail *st, int gpio){
    st->gpio = gpio;
    st->freq_hz = 1.0f;
    st->amp_deg = 30.0f;
    st->center_deg = 0.0f;
    st->us_min = 500; 
    st->us_center = 1500; 
    st->us_max = 2500;
    st->t = 0.0f; 
    st->running=false;
}
void st_set_profile(ServoTail *st, float f, float a, float c){ st->freq_hz=f; st->amp_deg=a; st->center_deg=c; }
void st_set_us_limits(ServoTail *st, uint16_t umin, uint16_t ucent, uint16_t umax){ st->us_min=umin; st->us_center=ucent; st->us_max=umax; }
void st_start(ServoTail *st){ st->running=true; st->t=0.0f; }
void st_stop (ServoTail *st){ st->running=false; }

uint16_t st_step_compute_pulse_us(ServoTail *st, float dt_sec){
    if(!st->running) return st->us_center;
    st->t += dt_sec;
    const float w = 2.0f * (float)M_PI * st->freq_hz;
    float deg = st->center_deg + st->amp_deg * sinf(w * st->t);
    return deg_to_us(st, deg);
}
