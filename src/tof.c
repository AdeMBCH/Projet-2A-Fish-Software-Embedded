
// FICHIER POUR UN SEUL TOF

// #include <string.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"

// #include "driver/i2c.h"
// #include "driver/gpio.h"

// #include "esp_log.h"
// #include "esp_err.h"

// #include "tof.h"

// #define TOF_I2C_PORT        I2C_NUM_0
// #define TOF_I2C_SDA_GPIO    GPIO_NUM_21
// #define TOF_I2C_SCL_GPIO    GPIO_NUM_22 
// #define TOF_I2C_CLK_HZ      400000        /* 400 kHz max */

// /* Adresse I2C 7 bits par défaut du VL53L0X */
// #define VL53L0X_I2C_ADDR                        0x29

// /* Registres principaux (cf. doc/register map ST) */
// #define VL53L0X_REG_SYSRANGE_START              0x00
// #define VL53L0X_REG_RESULT_RANGE_STATUS         0x14
// #define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR      0x0B
// #define VL53L0X_REG_IDENTIFICATION_MODEL_ID     0xC0

// /* Nombre d’octets à lire à partir de RESULT_RANGE_STATUS pour récupérer la distance */
// #define VL53L0X_RESULT_BLOCK_SIZE               12

// /* Temps typique d’une mesure single-shot (ordre de grandeur) */
// #define VL53L0X_MEASUREMENT_TIME_MS             30

// /* Période de la tâche (fréquence d’échantillonnage) */
// #define TOF_TASK_PERIOD_MS                      50

// static const char *TAG = "tof";

// static bool                 s_tof_initialized = false;
// static SemaphoreHandle_t    s_sample_mutex    = NULL;
// static tof_sample_t         s_last_sample     = { .distance_mm = 0, .valid = 0 };

// static esp_err_t tof_i2c_init(void)
// {
//     i2c_config_t conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = TOF_I2C_SDA_GPIO,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_io_num = TOF_I2C_SCL_GPIO,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = TOF_I2C_CLK_HZ,
//         .clk_flags = 0
//     };

//     esp_err_t err = i2c_param_config(TOF_I2C_PORT, &conf);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "i2c_param_config failed: %d", err);
//         return err;
//     }

//     err = i2c_driver_install(TOF_I2C_PORT, conf.mode, 0, 0, 0);
//     if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
//         ESP_LOGE(TAG, "i2c_driver_install failed: %d", err);
//         return err;
//     }

//     return ESP_OK;
// }

// static esp_err_t vl53_write8(uint8_t reg, uint8_t value)
// {
//     uint8_t buf[2] = { reg, value };
//     return i2c_master_write_to_device(
//         TOF_I2C_PORT,
//         VL53L0X_I2C_ADDR,
//         buf,
//         sizeof(buf),
//         pdMS_TO_TICKS(50)
//     );
// }

// static esp_err_t vl53_read_multi(uint8_t reg, uint8_t *data, size_t len)
// {
//     return i2c_master_write_read_device(
//         TOF_I2C_PORT,
//         VL53L0X_I2C_ADDR,
//         &reg,
//         1,
//         data,
//         len,
//         pdMS_TO_TICKS(50)
//     );
// }

// static esp_err_t vl53_read8(uint8_t reg, uint8_t *value)
// {
//     return vl53_read_multi(reg, value, 1);
// }

// esp_err_t tof_init(void)
// {
//     esp_err_t err;

//     if (s_tof_initialized) {
//         return ESP_OK;
//     }

//     err = tof_i2c_init();
//     if (err != ESP_OK) {
//         return err;
//     }

//     /* Vérification basique : lecture du model ID (doit être 0xEE sur VL53L0X) */
//     uint8_t model_id = 0;
//     err = vl53_read8(VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "VL53L0X not responding on I2C (err=%d)", err);
//         return err;
//     }

//     if (model_id != 0xEE) {
//         ESP_LOGW(TAG, "Unexpected model ID: 0x%02X (expected 0xEE), check sensor/module", model_id);
//         /* On continue quand même, certains clones ne respectent pas tout à la lettre. */
//     }

//     if (s_sample_mutex == NULL) {
//         s_sample_mutex = xSemaphoreCreateMutex();
//         if (s_sample_mutex == NULL) {
//             ESP_LOGE(TAG, "Failed to create sample mutex");
//             return ESP_ERR_NO_MEM;
//         }
//     }

//     s_last_sample.distance_mm = 0;
//     s_last_sample.valid       = 0;

//     s_tof_initialized = true;
//     ESP_LOGI(TAG, "VL53L0X driver initialized");
//     return ESP_OK;
// }

// esp_err_t tof_read_blocking(uint16_t *distance_mm)
// {
//     if (distance_mm == NULL) {
//         return ESP_ERR_INVALID_ARG;
//     }

//     esp_err_t err;

//     /* Démarrage d’une mesure single-shot */
//     err = vl53_write8(VL53L0X_REG_SYSRANGE_START, 0x01);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to start ranging: %d", err);
//         return err;
//     }

//     /* Temps de conversion typique (~30 ms) */
//     vTaskDelay(pdMS_TO_TICKS(VL53L0X_MEASUREMENT_TIME_MS));

//     uint8_t buf[VL53L0X_RESULT_BLOCK_SIZE];
//     err = vl53_read_multi(VL53L0X_REG_RESULT_RANGE_STATUS, buf, sizeof(buf));
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to read RESULT_RANGE_STATUS block: %d", err);
//         return err;
//     }

//     /*
//      * D’après les docs d’exemple (M5Stack, etc.), la distance est contenue
//      * dans buf[10] (LSB) et buf[11] (MSB), en millimètres.
//      */
//     uint16_t dist = ((uint16_t)buf[10] << 8) | buf[11];

//     /* Clear éventuel de l’interruption "new sample ready" côté capteur */
//     (void)vl53_write8(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

//     *distance_mm = dist;
//     return ESP_OK;
// }

// void tof_get_last_sample(tof_sample_t *sample)
// {
//     if (sample == NULL) {
//         return;
//     }

//     if (s_sample_mutex == NULL) {
//         sample->distance_mm = 0;
//         sample->valid       = 0;
//         return;
//     }

//     if (xSemaphoreTake(s_sample_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
//         *sample = s_last_sample;
//         xSemaphoreGive(s_sample_mutex);
//     } else {
//         sample->distance_mm = 0;
//         sample->valid       = 0;
//     }
// }

// void tof_task(void *pvParameters)
// {
//     (void)pvParameters;

//     esp_err_t err = tof_init();
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "tof_init failed in tof_task: %d, task will stop", err);
//         vTaskDelete(NULL);
//         return;
//     }

//     ESP_LOGI(TAG, "tof_task started");

//     for (;;)
//     {
//         uint16_t dist_mm = 0;
//         tof_sample_t sample = {0};

//         err = tof_read_blocking(&dist_mm);
//         if (err == ESP_OK) {
//             sample.distance_mm = dist_mm;
//             sample.valid       = 1;
//             ESP_LOGD(TAG, "Distance: %u mm", dist_mm);
//             ESP_LOGI(TAG, "TOF distance = %u mm", dist_mm);
//         } else {
//             sample.distance_mm = 0;
//             sample.valid       = 0;
//             ESP_LOGW(TAG, "Distance read error: %d", err);
//         }

//         if (s_sample_mutex != NULL) {
//             if (xSemaphoreTake(s_sample_mutex, portMAX_DELAY) == pdTRUE) {
//                 s_last_sample = sample;
//                 xSemaphoreGive(s_sample_mutex);
//             }
//         }

//         vTaskDelay(pdMS_TO_TICKS(TOF_TASK_PERIOD_MS));
//     }
// }
