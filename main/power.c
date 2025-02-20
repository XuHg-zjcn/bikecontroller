/*
 * power.c: 电源管理
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
//some code copy from esp-idf/examples/system/light_sleep/main/light_sleep_example_main.c
#include "power.h"
#include "u8g2_user.h"
#include "esp_check.h"
#include "esp_sleep.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "power";

#define PIN_NUM_CLR   CONFIG_HC595_PIN_CLR
#define PIN_NUM_RCLK  CONFIG_HC595_PIN_RCLK

esp_err_t power_set_walkup_pin(int pin_num, int trig_stat)
{
  /*ESP_RETURN_ON_ERROR(gpio_wakeup_enable(pin_num, trig_stat == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL),
    TAG, "Enable gpio wakeup failed");*/
  ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), TAG, "Configure gpio as wakeup source failed");
//ESP_LOGI(TAG, "gpio wakeup source is ready on PIN%d", pin_num);
  return ESP_OK;
}

void power_light_sleep()
{
  lcd_bl_off();
  ESP_LOGI(TAG, "Ready entry light sleep");
  gpio_set_level(PIN_NUM_CLR, 0);
  gpio_set_level(PIN_NUM_RCLK, 0); //先设置74HC595再休眠能维持电平输出，LCD不会被复位
  uart_wait_tx_idle_polling(UART_NUM_0);
  gpio_wakeup_enable(21, GPIO_INTR_LOW_LEVEL); //经测试，UART唤醒无效，改用GPIO唤醒
  esp_sleep_enable_gpio_wakeup();
  int64_t t_before_us = esp_timer_get_time();
  esp_light_sleep_start();
  int64_t t_after_us = esp_timer_get_time();
  gpio_wakeup_disable(21);

  ESP_LOGI(TAG, "Returned from light sleep, reason: ***, now t=%d ms, slept for %d ms",
	   ((int) (t_after_us / 1000)),
	   ((int) ((t_after_us - t_before_us) / 1000)));
  lcd_bl_on();
}
