/*
 * 轮速计驱动程序
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include <math.h>
#include "u8g2.h"

static const char *TAG = "wheelspeed";

#define GPIO_NUM_HALL  12
#define RESOLUTION_HZ  1000000 // 1MHz
#define METER_PER_CNT  1.52f

/**
 * @brief User defined context, to be passed to GPIO ISR callback function.
 */
typedef struct {
    TaskHandle_t task_to_notify;
    gpio_num_t echo_gpio;
} gpio_callback_user_data_t;

/**
 * @brief GPIO ISR callback function, called when there's fall edge detected on the GPIO of hall.
 */
static void hall_callback(void *user_data)
{
    static uint64_t timer_samp_old;
    uint64_t timer_samp_curr = esp_timer_get_time();
    gpio_callback_user_data_t *callback_user_data = (gpio_callback_user_data_t *)user_data;
    TaskHandle_t task_to_notify = callback_user_data->task_to_notify;

    //gptimer_get_captured_count(gptimer, &timer_samp_curr);
    uint64_t ticks_hall = (uint32_t)(timer_samp_curr - timer_samp_old);
    if(ticks_hall > UINT32_MAX){
      ticks_hall = UINT32_MAX;
    }
    timer_samp_old = timer_samp_curr;

    xTaskNotifyFromISR(task_to_notify, ticks_hall, eSetValueWithOverwrite, NULL);
}

void wheel_speed(u8g2_t *u8g2)
{
    ESP_LOGI(TAG, "Configure hall gpio");
    gpio_config_t echo_io_conf = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE, // capture signal on fall edge
        .pull_up_en = true, // pull up internally
        .pin_bit_mask = 1ULL << GPIO_NUM_HALL,
    };
    ESP_ERROR_CHECK(gpio_config(&echo_io_conf));

    ESP_LOGI(TAG, "Install GPIO edge interrupt");
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    gpio_callback_user_data_t callback_user_data = {
        .echo_gpio = GPIO_NUM_HALL,
        .task_to_notify = xTaskGetCurrentTaskHandle(),
    };
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_HALL, hall_callback, &callback_user_data));

    int hall_cnt = 0;
    int show_km = 0;
    uint32_t ticks_hall;
    char temp_str[16];
    ESP_LOGI(TAG, "entry loop");
    while (1) {
      // wait for echo done signal
      if (xTaskNotifyWait(0x00, ULONG_MAX, &ticks_hall, pdMS_TO_TICKS(1000)) == pdTRUE) {
	if (ticks_hall < RESOLUTION_HZ/50 || ticks_hall > RESOLUTION_HZ*30) {
	  // out of range
	  continue;
	}
	hall_cnt++;
	float speed_kmh = (METER_PER_CNT*3.6*RESOLUTION_HZ)/(float)ticks_hall;

	ESP_LOGI(TAG, "hall_cnt: %d, speed: %.2fkm/h", hall_cnt, speed_kmh);
	int speed_show = (int)roundf(speed_kmh);
	if(speed_show < 0){
	  speed_show = 0;
	}
	if(speed_show > 99){
	  speed_show = 99;
	}
	snprintf(temp_str, 16, "%2d", speed_show);
	u8g2_SetFont(u8g2, u8g2_font_spleen32x64_mf);
	u8g2_DrawStr(u8g2, 0, 56, temp_str);

	int show_km_curr = (int)roundf(METER_PER_CNT*hall_cnt/10.0);
	if(show_km_curr != show_km){
	  snprintf(temp_str, 16, "%6.2f", METER_PER_CNT*hall_cnt/1000.0);
	  u8g2_SetFont(u8g2, u8g2_font_spleen8x16_mf);
	  u8g2_DrawStr(u8g2, 65, 16, temp_str);
	  show_km = show_km_curr;
	}
	u8g2_SendBuffer(u8g2);
      }
    }
}
