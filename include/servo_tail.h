#ifndef SERVO_TAIL_H
#define SERVO_TAIL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int   gpio;          // ex: 18 (signal)
    float freq_hz;       // Hz
    float amp_deg;       // amplitude crête
    float center_deg;    // offset
    uint16_t us_min;     // 500
    uint16_t us_center;  // 1500
    uint16_t us_max;     // 2500
    // interne
    float t;
    bool  running;
} ServoTail;

void     st_init(ServoTail *st, int gpio);
void     st_set_profile(ServoTail *st, float freq_hz, float amp_deg, float center_deg);
void     st_set_us_limits(ServoTail *st, uint16_t us_min, uint16_t us_center, uint16_t us_max);
void     st_start(ServoTail *st);
void     st_stop (ServoTail *st);
uint16_t st_step_compute_pulse_us(ServoTail *st, float dt_sec);



#endif 