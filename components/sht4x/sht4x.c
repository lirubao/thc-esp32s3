#include "sht4x.h"

void sht4x_init(sht4x_config_t *config) {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = config->sda;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = config->scl;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = config->freq_hz;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    esp_err_t ret = i2c_param_config(config->i2c_num, &conf);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C config returned error");

    ret = i2c_driver_install(config->i2c_num, conf.mode, 0, 0, 0);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C install returned error");
}

sht4x_t read_sht4x_data() {
    sht4x_t rs;
    int sht4x_address = 0x44;
    uint8_t c = 0xFD; //高精度（高重复性）测量
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (sht4x_address << 1 | I2C_MASTER_WRITE), true);

    i2c_master_write(cmd, &c, 1, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) { loge("sht4x", "%dI2C write failed", ret); }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t data[6] = {0, 0, 0, 0, 0, 0};

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (sht4x_address << 1 | I2C_MASTER_READ), true);
    for (int i = 0; i < 6; i++) { i2c_master_read_byte(cmd, &data[i], I2C_MASTER_ACK); }

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        // for (int i = 0; i < 6; i++) { printf(" %d ", data[i]); }
        // printf("\n");
        float t_ticks = data[0] << 8 | data[1];
        float rh_ticks = data[3] << 8 | data[4];
        rs.temp = -45 + 175.0 * t_ticks / 65535.0;
        rs.hum = -6 + 125.0 * rh_ticks / 65535.0;
    }
    return rs;
}