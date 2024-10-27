/*
 * U8G2用户文件
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
#include "u8g2_user.h"
#include "u8g2.h"
#include "u8x8.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_rom_sys.h"
#include <string.h>
#include <math.h>

#define LCD_HOST    SPI3_HOST
#define PIN_NUM_MISO CONFIG_LCD_PIN_MISO //unused
#define PIN_NUM_MOSI CONFIG_LCD_PIN_MOSI
#define PIN_NUM_CLK  CONFIG_LCD_PIN_CLK
#define PIN_NUM_CS   CONFIG_LCD_PIN_CS

#define PIN_NUM_DC   CONFIG_LCD_PIN_DC
#define PIN_NUM_RST  CONFIG_LCD_PIN_RST
#define PIN_NUM_BL   CONFIG_LCD_PIN_BL

static spi_device_handle_t dev;

void lcd_bl_off()
{
  gpio_set_level(PIN_NUM_BL, 0);
}

void lcd_bl_on()
{
  gpio_set_level(PIN_NUM_BL, 1);
}

static void st7567_gpio_spi_init()
{
  esp_err_t ret;
  spi_bus_config_t buscfg = {
    .miso_io_num = PIN_NUM_MISO,
    .mosi_io_num = PIN_NUM_MOSI,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 256,
  };
  spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 1 *1000 * 1000,       //Clock out at 10 MHz
    .mode = 3,                              //SPI mode 3
    .spics_io_num = PIN_NUM_CS,             //CS pin
    .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
  };
  //Initialize the SPI bus
  ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(LCD_HOST, &devcfg, &dev);
  ESP_ERROR_CHECK(ret);

  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BL);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = false;
  io_conf.pull_up_en = true;
  gpio_config(&io_conf);

  gpio_set_level(PIN_NUM_RST, 0);
  esp_rom_delay_us(10);
  gpio_set_level(PIN_NUM_RST, 1);
  esp_rom_delay_us(10);
}

static uint8_t st7567_jlx12864_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg){
  case U8X8_MSG_BYTE_INIT:
    break;
  case U8X8_MSG_BYTE_SEND:
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = arg_int*8;
    t.tx_buffer = arg_ptr;
    ret = spi_device_polling_transmit(dev, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
    break;
  case U8X8_MSG_BYTE_SET_DC:
    gpio_set_level(PIN_NUM_DC, arg_int);
    esp_rom_delay_us(1);
    break;
  case U8X8_MSG_BYTE_START_TRANSFER:
    gpio_set_level(PIN_NUM_CS, 0);
    esp_rom_delay_us(1);
    break;
  case U8X8_MSG_BYTE_END_TRANSFER:
    gpio_set_level(PIN_NUM_CS, 1);
    esp_rom_delay_us(1);
    break;
  default:
    return 0;
  }
  return 1;
}

static uint8_t st7567_jlx12864_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg){
  case U8X8_MSG_GPIO_AND_DELAY_INIT:
    st7567_gpio_spi_init();
    break;
  case U8X8_MSG_DELAY_MILLI:
    vTaskDelay(arg_int/portTICK_PERIOD_MS);
    esp_rom_delay_us((arg_int/portTICK_PERIOD_MS)*1000);
    break;
  case U8X8_MSG_DELAY_NANO:
    esp_rom_delay_us(arg_int/1000);
    break;
  default:
    return 0;
  }
  return 1;
}

void u8g2_init(u8g2_t *u8g2)
{
  u8g2_Setup_st7567_jlx12864_f(u8g2, U8G2_R0, st7567_jlx12864_byte_cb, st7567_jlx12864_gpio_and_delay_cb);
  u8g2_InitDisplay(u8g2);
  u8g2_SetPowerSave(u8g2, 0);
  u8g2_ClearBuffer(u8g2);
  u8g2_SendBuffer(u8g2);
  lcd_bl_on();
}

void u8g2_show(u8g2_t *u8g2, float speed_kmh, float dist_km)
{
  char temp_str[16];
  int speed_show = (int)roundf(speed_kmh);
  if(speed_show < 0)
    speed_show = 0;
  if(speed_show > 99)
    speed_show = 99;
  snprintf(temp_str, 16, "%2d", speed_show);
  u8g2_SetFont(u8g2, u8g2_font_spleen32x64_mf);
  u8g2_DrawStr(u8g2, 0, 56, temp_str);

  snprintf(temp_str, 16, "%6.2f", dist_km);
  u8g2_SetFont(u8g2, u8g2_font_spleen8x16_mf);
  u8g2_DrawStr(u8g2, 65, 16, temp_str);
  u8g2_SendBuffer(u8g2);
}

void u8g2_show_zero(u8g2_t *u8g2)
{
  u8g2_SetFont(u8g2, u8g2_font_spleen32x64_mf);
  u8g2_DrawStr(u8g2, 0, 56, " 0");
  u8g2_SendBuffer(u8g2);
}
