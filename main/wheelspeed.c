/*
 * 轮速计驱动程序
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include <stdio.h>
#include <math.h>
#include "u8g2_user.h"
#include "storage.h"
#include "power.h"

static const char *TAG = "wheelspeed";
static volatile uint64_t timer_samp;

#define RESOLUTION_HZ  20000 // 20KHz
#define METER_PER_CNT  1.52f
#define SHOW_TIMEOUT_MS (METER_PER_CNT/(0.5/3.6)*1000)


void wheel_speed(u8g2_t *u8g2)
{
    uint32_t hall_cnt = 0;
    uint32_t ticks_hall = 0;
    ESP_LOGI(TAG, "entry loop");
    while (1) {
      if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &ticks_hall, pdMS_TO_TICKS(SHOW_TIMEOUT_MS)) == pdTRUE) {
	if (ticks_hall < RESOLUTION_HZ/50) {
	  //too short
	  continue;
	}
	hall_cnt++;
	float speed_kmh = (METER_PER_CNT*3.6*RESOLUTION_HZ)/(float)ticks_hall;
	float dist_km = METER_PER_CNT*hall_cnt/1000.0;
	ESP_LOGI(TAG, "hall_cnt: %lu, ticks=%lu, speed: %.2fkm/h", hall_cnt, ticks_hall, speed_kmh);
	u8g2_show(u8g2, speed_kmh, dist_km);
	//storage_record_wheelspeed(hall_cnt, ticks_hall);
      }else{
	u8g2_show_zero(u8g2);
	power_light_sleep();
      }
    }
}
