/*
 * LED驱动
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
#include "led.h"
#include "driver/gpio.h"

void led_init()
{
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << 2);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = false;
  gpio_config(&io_conf);
}

void led_on()
{
  gpio_set_level(2, 1);
}

void led_off()
{
  gpio_set_level(2, 0);
}
