#ifndef SERVO_HAL_H
#define SERVO_HAL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int gpio;                // 18
    int timer;               // LEDC_TIMER_0..3
    int channel;             // LEDC_CHANNEL_0..7
    int pwm_freq_hz;         // 50
    int duty_res_bits;       // 16
} ServoHALCfg;

typedef struct {
    ServoHALCfg cfg;
    bool ready;
    unsigned max_duty;       // (1<<res)-1
} ServoHAL;

bool servo_hal_init(ServoHAL *h, const ServoHALCfg *cfg);
bool servo_hal_write_pulse_us(ServoHAL *h, uint16_t pulse_us); // 500..2500 us
void servo_hal_deinit(ServoHAL *h);


#endif