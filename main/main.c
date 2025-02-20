/*
 * 自行车控制器主程序文件
 * Copyright (C) 2024-2025  徐瑞骏(科技骏马)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "tcp_client_v4.h"
#include "led.h"
#include "u8g2_user.h"
#include "wifi.h"
#include "wheelspeed.h"
#include "storage.h"
#include "sdcard.h"
#include "i2c.h"
#include "mpu9250.h"
#include "record.h"
#include "spi.h"
#include "74hc595.h"
#include "uart.h"
#include "power.h"

static const char *TAG = "main";

TaskHandle_t task_wheelspeed;
u8g2_t u8g2;

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    led_init();
    i2c_master_init();
    spi_init();
    hc595_init();
    UART_Init();
    power_init();
    //sdcard_init();
    u8g2_init(&u8g2);
    u8g2_DrawFrame(&u8g2, 0, 0, 128, 64);
    u8g2_SetFont(&u8g2, u8g2_font_spleen8x16_mf);
    u8g2_DrawStr(&u8g2, 70, 56, "km/h");
    u8g2_DrawStr(&u8g2, 64, 16, "  0.00");
    u8g2_DrawStr(&u8g2, 113, 16, "km");
    u8g2_SendBuffer(&u8g2);
    ESP_LOGI(TAG, "init finish");
    //littlefs_init();
    //mpu9250_init();
    xTaskCreate((void (*)(void *))wheel_speed, "whell_speed", 4096, &u8g2, 12, &task_wheelspeed);
    /*xTaskCreatePinnedToCore((void (*)(void *))mpu9250_print_data, "mpu9250", 4096, NULL, 3, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore((void (*)(void *))record_proc, "record", 4096, NULL, 3, NULL, tskNO_AFFINITY);*/
    
    //ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    //wifi_init_sta();
    //xTaskCreatePinnedToCore((void (*)(void *))client_fn, "client", 4096, NULL, 3, NULL, tskNO_AFFINITY);
    while(1){
      vTaskDelay(pdMS_TO_TICKS(200));
      u8g2_show_mag(&u8g2);
      u8g2_SendBuffer(&u8g2);
    }
}
