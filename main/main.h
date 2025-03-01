#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "ssd1306_fonts.h"
#include "ssd1306.h"
#include "sht4x.h"
#include "ap_sta_wifi.h"

#define GPIO_DEFAULT GPIO_NUM_4
#endif