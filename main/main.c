#include "main.h"

void gpio_default_init() {
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << GPIO_DEFAULT),
                             .mode = GPIO_MODE_OUTPUT,
                             .pull_up_en = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

const int HUMI_BIG = 63;
const int HUMI_SMALL = 60;

void is_hum(float hum) {
    if (hum < HUMI_SMALL) gpio_set_level(GPIO_DEFAULT, 1);
    if (hum > HUMI_BIG) gpio_set_level(GPIO_DEFAULT, 0);
}

QueueHandle_t xQueue = NULL;

void task_sht4x(void *pvParameters) {
    sht4x_t data;

    while (1) {
        data = read_sht4x_data();
        xQueueSend(xQueue, &data, portMAX_DELAY);
    }
}
void task_display(void *pvParameters) {
    ssd1306_handle_t dev = ssd1306_create(I2C_NUM_0, SSD1306_I2C_ADDRESS);
    ssd1306_refresh_gram(dev);
    ssd1306_clear_screen(dev, 0x00);
    sht4x_t data;

    while (1) {
        xQueueReceive(xQueue, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(50));
        logi("sht4x", "温度：%.2f 湿度：%.2f\n", data.temp, data.hum);
        char temp[20], hum[20];
        is_hum(data.hum);
        sprintf(temp, "tem: %.2f", data.temp);
        sprintf(hum, "hum: %.2f", data.hum);
        ssd1306_draw_string(dev, 0, 5, (const uint8_t *)temp, 16, 1);
        ssd1306_draw_string(dev, 0, 25, (const uint8_t *)hum, 16, 1);
        ssd1306_refresh_gram(dev);
    }
}

// 主函数
void app_main(void) {
    sht4x_config_t config = {.sda = 5, .scl = 6, .i2c_num = I2C_NUM_0, .freq_hz = 100000};
    sht4x_init(&config);
    gpio_default_init();

    xQueue = xQueueCreate(10, sizeof(sht4x_t));
    xTaskCreatePinnedToCore(task_sht4x, "task_sht4x", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_display, "task_display", 2048, NULL, 5, NULL, 1);
    // start_wifi();
}