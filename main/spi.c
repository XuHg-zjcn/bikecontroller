/*
 * SPI驱动程序
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
#include "driver/spi_master.h"

#define SPI_HOST     SPI2_HOST
#define PIN_NUM_MISO CONFIG_SPI_PIN_MISO
#define PIN_NUM_MOSI CONFIG_SPI_PIN_MOSI
#define PIN_NUM_SCLK CONFIG_SPI_PIN_CLK


esp_err_t spi_init(void)
{
  spi_bus_config_t buscfg = {
    .miso_io_num = PIN_NUM_MISO,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 256,
  };
  return spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
}
