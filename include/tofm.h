#ifndef TOFM_H
#define TOFM_H

#include <stdint.h>
#include "esp_err.h"

// Nombre de capteurs ToF 
#define TOF_NUM_SENSORS 3

// Configuration d'un capteur ToF
typedef struct
{
    uint8_t  i2c_addr;      // Adresse I2C finale (0x30, 0x31, 0x29...)
    int      xshut_gpio;    // GPIO pour XSHUT
    char     name[16];      // Nom du capteur pour les logs
} tof_config_t;

// Échantillon de mesure
typedef struct
{
    uint16_t distance_mm;   // Dernière distance valide en mm 
    uint8_t  valid;         // 1 si la dernière mesure est valide, 0 sinon 
} tof_sample_t;


esp_err_t tof_init(void);

esp_err_t tof_read_blocking(uint8_t sensor_index, uint16_t *distance_mm);

void tof_get_last_sample(uint8_t sensor_index, tof_sample_t *sample);

void tof_task(void *pvParameters);

#endif