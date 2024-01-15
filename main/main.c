/*
 * 自行车控制器主程序文件
 * Copyright (C) 2024  徐瑞骏(科技骏马)
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

static const char *TAG = "main";

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
    u8g2_init(&u8g2);
    u8g2_DrawFrame(&u8g2, 4, 4, 128-10, 64-10);
    u8g2_SetFont(&u8g2, u8g2_font_spleen32x64_mf);
    u8g2_DrawStr(&u8g2, 0, 48, "15");
    u8g2_SetFont(&u8g2, u8g2_font_spleen8x16_mf);
    u8g2_DrawStr(&u8g2, 70, 48, "km/h");
    u8g2_SendBuffer(&u8g2);
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    tcp_client();
}
