#include "servo_ctrl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

bool sc_init(ServoCtrl *sc, int gpio){
    if(!sc) return false;
    st_init(&sc->model, gpio);
    sc->dt_sec = 0.020f;
    sc->running = false;

    ServoHALCfg cfg = {
        .gpio = gpio,
        .timer = LEDC_TIMER_0,
        .channel = LEDC_CHANNEL_0,
        .pwm_freq_hz = 50,
        .duty_res_bits = 16
    };
    servo_hal_init(&sc->hal, &cfg);
    return true;
}
void sc_set_profile(ServoCtrl *sc, float f, float a, float c){ st_set_profile(&sc->model, f, a, c); }
void sc_start(ServoCtrl *sc){ st_start(&sc->model); sc->running=true; }
void sc_stop (ServoCtrl *sc){ st_stop (&sc->model); sc->running=false; }

void servo_task(void *arg){
    ServoCtrl *sc = (ServoCtrl*)arg;
    const TickType_t dt = pdMS_TO_TICKS((int)(sc->dt_sec * 1000.0f)); // 20 ms
    for(;;){
        uint16_t us = st_step_compute_pulse_us(&sc->model, sc->dt_sec);
        servo_hal_write_pulse_us(&sc->hal, us);
        vTaskDelay(dt);
    }
}
