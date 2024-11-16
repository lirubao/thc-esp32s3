#include <stdio.h>
#include "sdkconfig.h"

#include "sht4x.h"

// 主函数
void app_main(void) {
    sht4x_config_t config = {.sda = 5, .scl = 6, .i2c_num = I2C_NUM_0, .freq_hz = 100000};
    sht4x_init(&config);
    while (1) {
        sht4x_t data = read_sht4x_data();
        printf("温度：%.2lf 温度：%.2lf\n", data.temp, data.hum);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}