#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include <stdbool.h>
#include "servo_tail.h"
#include "servo_hal.h"

typedef struct {
    ServoTail model;
    ServoHAL  hal;
    float     dt_sec;     // 0.02
    bool      running;
} ServoCtrl;

bool sc_init(ServoCtrl *sc, int gpio);
void sc_set_profile(ServoCtrl *sc, float freq_hz, float amp_deg, float center_deg);
void sc_start(ServoCtrl *sc);
void sc_stop(ServoCtrl *sc);

// Task FreeRTOS (boucle 50 Hz)
void servo_task(void *arg);

#endif
