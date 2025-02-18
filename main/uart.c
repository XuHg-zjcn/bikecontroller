/*
 * 串口驱动程序
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
 * along with this program  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "uart.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "misc.h"

#define UART_PORT_NUM  UART_NUM_0
#define UART_BAUD_RATE   115200
#define DEFAULT_PIN_RXD  20
#define DEFAULT_PIN_TXD  21
#define PATTERN_CHR      '+'
#define PATTERN_CHR_NUM  (3)

#define UBUFF_SIZE       256
#define QUEUE_SIZE       20

#define BUF_SIZE         (1024)
#define RD_BUF_SIZE      (BUF_SIZE)

static QueueHandle_t uart0_queue;
static const char *TAG = "uart";

extern int16_t mpu9250_data[9];

esp_err_t UART_direct_inv()
{
  return uart_set_pin(UART_PORT_NUM, DEFAULT_PIN_RXD, DEFAULT_PIN_TXD, 0, 0);
}

esp_err_t UART_direct_norm()
{
  return uart_set_pin(UART_PORT_NUM, DEFAULT_PIN_TXD, DEFAULT_PIN_RXD, 0, 0);
}


void UART_proc_rx(uint8_t *p, size_t len)
{
  while(len > 0){
    if(len>=8 && strncmp((const char *)p, "HatD", 4)==0){
      uint32_t ticks = READ_UINT32_UNALIGN(p+4);
      ESP_LOGI(TAG, "hall cap %4lu.%02lu ms", ticks/100, ticks%100);
      p += 8;
      len -= 8;
    }else if(len>=10 && strncmp((const char *)p, "MAGd", 4)==0){
      int16_t mx = READ_INT16_UNALIGN(p+4);
      int16_t my = READ_INT16_UNALIGN(p+6);
      int16_t mz = READ_INT16_UNALIGN(p+8);
      mpu9250_data[6] = mx;
      mpu9250_data[7] = my;
      mpu9250_data[8] = mz;
      ESP_LOGI(TAG, "mag x=%5d, y=%5d, z=%5d", mx, my, mz);
      p += 10;
      len -= 10;
    }else{
      uart_flush_input(UART_PORT_NUM);
      break;
    }
  }
}

/* reference:
 * https://github.com/espressif/esp-idf
 * examples/peripherals/uart/uart_events/main/uart_events_example_main.c
 */
static void uart_event_task(void *arg)
{
  uint8_t *ubuff;
  uart_event_t event;
  ubuff = (uint8_t *)malloc(UBUFF_SIZE);
  while(1){
    if(xQueueReceive(uart0_queue, &event, pdMS_TO_TICKS(1000))) {
      switch(event.type){
      case UART_DATA:
	size_t len = uart_read_bytes(UART_PORT_NUM, ubuff, UBUFF_SIZE-1, 10 / portTICK_PERIOD_MS);
	if(len > 0){
	  UART_proc_rx(ubuff, len);
	}
	break;
      case UART_FIFO_OVF:
	ESP_LOGI(TAG, "hw fifo overflow");
	uart_flush_input(UART_PORT_NUM);
	xQueueReset(uart0_queue);
	break;
      case UART_BUFFER_FULL:
	ESP_LOGI(TAG, "ring buffer full");
	uart_flush_input(UART_PORT_NUM);
	xQueueReset(uart0_queue);
	break;
      case UART_BREAK:
	ESP_LOGI(TAG, "uart rx break");
	break;
      case UART_PARITY_ERR:
	ESP_LOGI(TAG, "uart parity error");
	break;
      case UART_FRAME_ERR:
	ESP_LOGI(TAG, "uart frame error");
	break;
      case UART_PATTERN_DET:
	ESP_LOGI(TAG, "uart pattern detected");
	break;
      default:
	ESP_LOGI(TAG, "uart event type: %d", event.type);
	break;
      }
    }
  }
}

void UART_Init()
{
  esp_log_level_set("*", ESP_LOG_NONE);
  gpio_reset_pin(GPIO_NUM_20);
  gpio_reset_pin(GPIO_NUM_21);

  uart_config_t uart_config = {
    .baud_rate = UART_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  uart_driver_install(UART_PORT_NUM, BUF_SIZE, BUF_SIZE, QUEUE_SIZE, &uart0_queue, 0);
  uart_param_config(UART_PORT_NUM, &uart_config);
  esp_log_level_set(TAG, ESP_LOG_INFO);
  UART_direct_inv();

  xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}
