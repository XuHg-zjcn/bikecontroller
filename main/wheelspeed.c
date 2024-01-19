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
#include "u8g2.h"
#include "storage.h"

static const char *TAG = "wheelspeed";
static volatile uint64_t timer_samp;

#define GPIO_NUM_HALL  12
#define RESOLUTION_HZ  100000 // 100KHz
#define METER_PER_CNT  1.52f
#define CNT_PER_FLUSH  32

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
    int show_km = 0;
    char temp_str[16];
    uint64_t ts_old=0, ts_curr=0;
    char fname[16];
    FILE *fp = NULL;
    ESP_LOGI(TAG, "entry loop");
    while (1) {
      // wait for echo done signal
      if (xTaskNotifyWait(0x00, ULONG_MAX, &hall_cnt, pdMS_TO_TICKS(1000)) == pdTRUE) {
	ts_curr = timer_samp/10;
	uint64_t ticks_hall = ts_curr - ts_old;
	uint32_t ticks_hall_u32 = (ticks_hall>UINT32_MAX)?UINT32_MAX:(uint32_t)ticks_hall;
	if (ticks_hall > UINT32_MAX && fp != NULL){
	  fclose(fp);
	  fp = NULL;
	}
	if (ticks_hall < RESOLUTION_HZ/50) {
	  //too short
	  continue;
	}
	float speed_kmh = (METER_PER_CNT*3.6*RESOLUTION_HZ)/(float)ticks_hall;
	ESP_LOGI(TAG, "hall_cnt: %lu, speed: %.2fkm/h", hall_cnt, speed_kmh);
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
	if(ticks_hall < UINT32_MAX){
	  if(fp == NULL){
	    ESP_LOGI(TAG, "Getting filename");
	    storage_get_next_filename(fname);
	    ESP_LOGI(TAG, "Opening file %s", fname);
	    fp = fopen(fname, "w");
	    if(fp == NULL){
	      ESP_LOGE(TAG, "Open Failed");
	    }
	  }
	  if(fp != NULL){
	    fwrite(&ticks_hall_u32, sizeof(uint32_t), 1, fp);
	    if(hall_cnt % CNT_PER_FLUSH == 0){
	      ESP_LOGI(TAG, "Flush data");
	      if(fflush(fp) != 0){
	        ESP_LOGE(TAG, "Flush Failed");
	      }
	    }
	  }
	}
	ts_old = ts_curr;
      }
    }
}
