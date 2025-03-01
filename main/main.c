#include "main.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (17) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)                // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (5000)           // Frequency in Hertz. Set frequency at 4 kHz

#define MOTOR_GPIO_PIN GPIO_NUM_7                           // 假设电机控制引脚为 GPIO7
#define TWO_HOURS (2 * 60 * 60 * 1000 / portTICK_PERIOD_MS) // 2小时的毫秒数
#define THREE_MINUTES (3 * 60 * 1000 / portTICK_PERIOD_MS)  // 3分钟的毫秒数

void motor_control(bool on) {
    gpio_set_level(MOTOR_GPIO_PIN, on ? 1 : 0); // 打开电机
}
void motor_task(void *pvParameter) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,              // 设置为输出模式
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // 不启用下拉电阻
        .pull_up_en = GPIO_PULLDOWN_DISABLE,   // 不启用上拉电阻
        .intr_type = GPIO_INTR_DISABLE         // 禁用中断
    };
    gpio_config(&io_conf); // 配置GPIO引脚

    while (1) {
        motor_control(true);                   // 打开电机
        vTaskDelay(THREE_MINUTES);             // 等待3分钟
        motor_control(false);                  // 关闭电机
        vTaskDelay(TWO_HOURS - THREE_MINUTES); // 等待2小时减去3分钟
    }
}

void heat_init() {
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                      .timer_num = LEDC_TIMER,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
                                      .clk_cfg = LEDC_AUTO_CLK};
    check_err(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {.speed_mode = LEDC_MODE,
                                          .channel = LEDC_CHANNEL,
                                          .timer_sel = LEDC_TIMER,
                                          .intr_type = LEDC_INTR_DISABLE,
                                          .gpio_num = LEDC_OUTPUT_IO,
                                          .duty = 0, // Set duty to 0%
                                          .hpoint = 0};
    check_err(ledc_channel_config(&ledc_channel));
}

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
int duty = 0;

void task_sht4x(void *pvParameters) {
    sht4x_t data;

    heat_init();

    while (1) {
        data = read_sht4x_data();
        xQueueSend(xQueue, &data, portMAX_DELAY);
        if (data.temp < 37.8) {
            duty = LEDC_DUTY * 2;
        } else {
            duty = LEDC_DUTY / 2;
        }
        check_err(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
        // Update duty to apply the new value
        check_err(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
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
        logi("sht4x", "温度：%.2f 湿度：%.2f heat: %d\n", data.temp, data.hum, duty);
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
    xTaskCreatePinnedToCore(motor_task, "motor_task", 2048, NULL, 5, NULL, 3);
    // xTaskCreatePinnedToCore(task_heat, "task_heat", 2048, NULL, 5, NULL, 2);
    // start_wifi();
}