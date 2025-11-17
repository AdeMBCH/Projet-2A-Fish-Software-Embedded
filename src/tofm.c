#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"

#include "tofm.h"

// Configuration I2C
#define TOF_I2C_PORT        I2C_NUM_0
#define TOF_I2C_SDA_GPIO    GPIO_NUM_21
#define TOF_I2C_SCL_GPIO    GPIO_NUM_22 
#define TOF_I2C_CLK_HZ      400000

// Adresse I2C par défaut du VL53L0X
#define VL53L0X_DEFAULT_ADDR                    0x29

// Registres principaux
#define VL53L0X_REG_SYSRANGE_START              0x00
#define VL53L0X_REG_RESULT_RANGE_STATUS         0x14
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR      0x0B
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID     0xC0
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS    0x8A

// Paramètres de mesure
#define VL53L0X_RESULT_BLOCK_SIZE               12
#define VL53L0X_MEASUREMENT_TIME_MS             30
#define TOF_TASK_PERIOD_MS                      50

static const char *TAG = "tof";

// Configuration des 3 capteurs
static const tof_config_t s_tof_configs[TOF_NUM_SENSORS] = {
    { .i2c_addr = 0x30, .xshut_gpio = 25, .name = "ToF_1" },
    { .i2c_addr = 0x31, .xshut_gpio = 26, .name = "ToF_2" },
    { .i2c_addr = 0x32, .xshut_gpio = 27, .name = "ToF_3" }
};

// État global
static bool                 s_tof_initialized = false;
static SemaphoreHandle_t    s_sample_mutex[TOF_NUM_SENSORS] = {NULL};
static tof_sample_t         s_last_samples[TOF_NUM_SENSORS] = {0};

// ------------- Fonctions I2C bas niveau -----------

static esp_err_t tof_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOF_I2C_SDA_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = TOF_I2C_SCL_GPIO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TOF_I2C_CLK_HZ,
        .clk_flags = 0
    };

    esp_err_t err = i2c_param_config(TOF_I2C_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %d", err);
        return err;
    }

    err = i2c_driver_install(TOF_I2C_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %d", err);
        return err;
    }

    return ESP_OK;
}

static esp_err_t vl53_write8(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return i2c_master_write_to_device(
        TOF_I2C_PORT,
        addr,
        buf,
        sizeof(buf),
        pdMS_TO_TICKS(50)
    );
}

static esp_err_t vl53_read_multi(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        TOF_I2C_PORT,
        addr,
        &reg,
        1,
        data,
        len,
        pdMS_TO_TICKS(50)
    );
}

static esp_err_t vl53_read8(uint8_t addr, uint8_t reg, uint8_t *value)
{
    return vl53_read_multi(addr, reg, value, 1);
}

// Configuration XSHUT et changement d'adresse

static esp_err_t tof_xshut_init(void)
{
    // Configure tous les GPIO XSHUT en sortie, initialement à LOW
    for (int i = 0; i < TOF_NUM_SENSORS; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << s_tof_configs[i].xshut_gpio),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure XSHUT GPIO %d", s_tof_configs[i].xshut_gpio);
            return err;
        }
        gpio_set_level(s_tof_configs[i].xshut_gpio, 0);
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

static esp_err_t tof_change_address(uint8_t old_addr, uint8_t new_addr)
{
    // Les VL53L0X utilisent une adresse 7-bit décalée d'1 bit dans le registre
    esp_err_t err = vl53_write8(old_addr, VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS, new_addr & 0x7F);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to change I2C address from 0x%02X to 0x%02X", old_addr, new_addr);
        return err;
    }
    
    vTaskDelay(pdMS_TO_TICKS(5));
    return ESP_OK;
}

static esp_err_t tof_init_sensor(uint8_t index)
{
    const tof_config_t *cfg = &s_tof_configs[index];
    
    // Active le capteur via XSHUT
    gpio_set_level(cfg->xshut_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Vérifie le model ID à l'adresse par défaut
    uint8_t model_id = 0;
    esp_err_t err = vl53_read8(VL53L0X_DEFAULT_ADDR, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: VL53L0X not responding (err=%d)", cfg->name, err);
        return err;
    }
    
    if (model_id != 0xEE) {
        ESP_LOGW(TAG, "%s: Unexpected model ID 0x%02X (expected 0xEE)", cfg->name, model_id);
    }
    
    // Change l'adresse I2C si nécessaire
    if (cfg->i2c_addr != VL53L0X_DEFAULT_ADDR) {
        err = tof_change_address(VL53L0X_DEFAULT_ADDR, cfg->i2c_addr);
        if (err != ESP_OK) {
            return err;
        }
        
        // Vérifie que le capteur répond à la nouvelle adresse
        err = vl53_read8(cfg->i2c_addr, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "%s: Sensor not responding at new address 0x%02X", cfg->name, cfg->i2c_addr);
            return err;
        }
    }
    
    ESP_LOGI(TAG, "%s initialized at address 0x%02X", cfg->name, cfg->i2c_addr);
    return ESP_OK;
}

esp_err_t tof_init(void)
{
    if (s_tof_initialized) {
        return ESP_OK;
    }

    // Initialise le bus I2C
    esp_err_t err = tof_i2c_init();
    if (err != ESP_OK) {
        return err;
    }

    // Configure tous les XSHUT à LOW
    err = tof_xshut_init();
    if (err != ESP_OK) {
        return err;
    }

    // Initialise les capteurs séquentiellement
    for (int i = 0; i < TOF_NUM_SENSORS; i++) {
        err = tof_init_sensor(i);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize sensor %d", i);
            return err;
        }

        // Crée le mutex pour ce capteur
        s_sample_mutex[i] = xSemaphoreCreateMutex();
        if (s_sample_mutex[i] == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex for sensor %d", i);
            return ESP_ERR_NO_MEM;
        }

        s_last_samples[i].distance_mm = 0;
        s_last_samples[i].valid = 0;
    }

    s_tof_initialized = true;
    ESP_LOGI(TAG, "All %d VL53L0X sensors initialized", TOF_NUM_SENSORS);
    return ESP_OK;
}

esp_err_t tof_read_blocking(uint8_t sensor_index, uint16_t *distance_mm)
{
    if (sensor_index >= TOF_NUM_SENSORS || distance_mm == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t addr = s_tof_configs[sensor_index].i2c_addr;

    // Démarre une mesure single-shot
    esp_err_t err = vl53_write8(addr, VL53L0X_REG_SYSRANGE_START, 0x01);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor %d: Failed to start ranging", sensor_index);
        return err;
    }

    // Attend la fin de la mesure
    vTaskDelay(pdMS_TO_TICKS(VL53L0X_MEASUREMENT_TIME_MS));

    // Lit le bloc de résultats
    uint8_t buf[VL53L0X_RESULT_BLOCK_SIZE];
    err = vl53_read_multi(addr, VL53L0X_REG_RESULT_RANGE_STATUS, buf, sizeof(buf));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor %d: Failed to read results", sensor_index);
        return err;
    }

    // Extrait la distance (buf[10]=MSB, buf[11]=LSB)
    uint16_t dist = ((uint16_t)buf[10] << 8) | buf[11];

    // Clear l'interruption
    (void)vl53_write8(addr, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    *distance_mm = dist;
    return ESP_OK;
}

void tof_get_last_sample(uint8_t sensor_index, tof_sample_t *sample)
{
    if (sensor_index >= TOF_NUM_SENSORS || sample == NULL) {
        if (sample != NULL) {
            sample->distance_mm = 0;
            sample->valid = 0;
        }
        return;
    }

    if (s_sample_mutex[sensor_index] == NULL) {
        sample->distance_mm = 0;
        sample->valid = 0;
        return;
    }

    if (xSemaphoreTake(s_sample_mutex[sensor_index], pdMS_TO_TICKS(5)) == pdTRUE) {
        *sample = s_last_samples[sensor_index];
        xSemaphoreGive(s_sample_mutex[sensor_index]);
    } else {
        sample->distance_mm = 0;
        sample->valid = 0;
    }
}

void tof_task(void *pvParameters)
{
    (void)pvParameters;

    esp_err_t err = tof_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "tof_init failed: %d, task will stop", err);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "tof_task started, reading %d sensors", TOF_NUM_SENSORS);

    for (;;)
    {
        // Lit séquentiellement tous les capteurs
        for (int i = 0; i < TOF_NUM_SENSORS; i++) {
            uint16_t dist_mm = 0;
            tof_sample_t sample = {0};

            err = tof_read_blocking(i, &dist_mm);
            if (err == ESP_OK) {
                sample.distance_mm = dist_mm;
                sample.valid = 1;
                ESP_LOGI(TAG, "%s: %u mm", s_tof_configs[i].name, dist_mm);
            } else {
                sample.distance_mm = 0;
                sample.valid = 0;
                ESP_LOGW(TAG, "%s: read error %d", s_tof_configs[i].name, err);
            }

            // Stocke l'échantillon
            if (s_sample_mutex[i] != NULL) {
                if (xSemaphoreTake(s_sample_mutex[i], portMAX_DELAY) == pdTRUE) {
                    s_last_samples[i] = sample;
                    xSemaphoreGive(s_sample_mutex[i]);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TOF_TASK_PERIOD_MS));
    }
}