#ifndef __SHT4X_H__
#define __SHT4X_H__

#include "utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "unity.h"

typedef struct {
    int sda;
    int scl;
    i2c_port_t i2c_num;
    int freq_hz;
} sht4x_config_t;

typedef struct {
    float temp;
    float hum;
} sht4x_t;

void sht4x_init(sht4x_config_t *config);
sht4x_t read_sht4x_data();
#endif