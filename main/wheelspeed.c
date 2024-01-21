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

#include <stdio.h>
#include <math.h>
#include "u8g2_user.h"
#include "storage.h"

static const char *TAG = "wheelspeed";
static volatile uint64_t timer_samp;

#define GPIO_NUM_HALL  12
#define RESOLUTION_HZ  100000 // 100KHz
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
    static uint32_t count;
    timer_samp = esp_timer_get_time();
    gpio_callback_user_data_t *callback_user_data = (gpio_callback_user_data_t *)user_data;
    TaskHandle_t task_to_notify = callback_user_data->task_to_notify;
    count++;
    xTaskNotifyFromISR(task_to_notify, count, eSetValueWithOverwrite, NULL);
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

    uint32_t hall_cnt = 0;
    uint64_t ts_old=0, ts_curr=0;
    ESP_LOGI(TAG, "entry loop");
    while (1) {
      if (xTaskNotifyWait(0x00, ULONG_MAX, &hall_cnt, pdMS_TO_TICKS(1000)) == pdTRUE) {
	ts_curr = timer_samp/10;
	uint64_t ticks_hall = ts_curr - ts_old;
	if (ticks_hall < RESOLUTION_HZ/50) {
	  //too short
	  continue;
	}
	float speed_kmh = (METER_PER_CNT*3.6*RESOLUTION_HZ)/(float)ticks_hall;
	float dist_km = METER_PER_CNT*hall_cnt/1000.0;
	ESP_LOGI(TAG, "hall_cnt: %lu, speed: %.2fkm/h", hall_cnt, speed_kmh);
	u8g2_show(u8g2, speed_kmh, dist_km);
	storage_record_wheelspeed(hall_cnt, ticks_hall);
	ts_old = ts_curr;
      }
    }
}
