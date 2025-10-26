#include "servo_hal.h"
#include "driver/ledc.h"
#include "esp_err.h"

bool servo_hal_init(ServoHAL *h, const ServoHALCfg *cfg){
    if(!h || !cfg) return false;
    h->cfg = *cfg;
    h->ready = false;

    ledc_timer_config_t t = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = (ledc_timer_t)cfg->timer,
        .duty_resolution  = (ledc_timer_bit_t)cfg->duty_res_bits, // LEDC_TIMER_16_BIT
        .freq_hz          = cfg->pwm_freq_hz,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t ch = {
        .gpio_num   = cfg->gpio,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = (ledc_channel_t)cfg->channel,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = (ledc_timer_t)cfg->timer,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    h->max_duty = (1u << cfg->duty_res_bits) - 1u;
    h->ready = true;
    return true;
}

bool servo_hal_write_pulse_us(ServoHAL *h, uint16_t pulse_us){
    if(!h || !h->ready){
        return false;
    }
    const uint32_t period_us = 20000; // 50 Hz
    if(pulse_us > period_us){
        pulse_us = period_us;
    }
    uint32_t duty = (uint32_t)((uint64_t)pulse_us * h->max_duty / period_us);
    if(duty > h->max_duty){
        duty = h->max_duty;
    }

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)h->cfg.channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)h->cfg.channel));
    return true;
}

void servo_hal_deinit(ServoHAL *h){ (void)h; }
