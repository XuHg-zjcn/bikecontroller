/*
 * 74HC595驱动程序
 * Copyright (C) 2025  徐瑞骏(科技骏马)
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
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define SPI_HOST        SPI2_HOST
#define PIN_HC595_CLR   0
#define PIN_HC595_RCLK  4
static spi_device_handle_t dev;
static uint8_t hc595_curr = 0;

void hc595_init(void)
{
  esp_err_t ret;
  spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 2 * 1000 * 1000,       //Clock out at 2 MHz
    .mode = 3,                              //SPI mode 3
    .spics_io_num = -1,                     //CS pin
    .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
  };
  //Initialize the SPI bus
  ret = spi_bus_add_device(SPI_HOST, &devcfg, &dev);
  ESP_ERROR_CHECK(ret);
}

void hc595_write(uint8_t data)
{
  spi_transaction_t t;
  hc595_curr = data;
  gpio_set_level(PIN_HC595_CLR, 1);
  gpio_set_level(PIN_HC595_RCLK, 0);

  memset(&t, 0, sizeof(t));
  t.length = 8;
  t.tx_buffer = &data;
  spi_device_polling_transmit(dev, &t);

  esp_rom_delay_us(1);
  gpio_set_level(PIN_HC595_RCLK, 1);
  esp_rom_delay_us(1);
  gpio_set_level(PIN_HC595_CLR, 0);
}

uint8_t hc595_set(uint8_t mask)
{
  uint8_t new_data = hc595_curr | mask;
  hc595_write(new_data);
  return new_data;
}

uint8_t hc595_reset(uint8_t mask)
{
  uint8_t new_data = hc595_curr & (~mask);
  hc595_write(new_data);
  return new_data;
}
