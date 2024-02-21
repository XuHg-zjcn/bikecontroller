/*
 * TCP客户端程序
 * Copyright (C) 2024  徐瑞骏(科技骏马)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"
#include "led.h"
#if defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
#include "addr_from_stdin.h"
#endif

#define HOST_IP_ADDR "192.168.1.18"
#define PORT 3234

static const char *TAG = "tcp_client_v4";
static const char hello[] = "hello";

extern EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

typedef enum{
  Cmd_FileOpen = 0,
  Cmd_FileClose,
  Cmd_FileRead,
  Cmd_FileDelete,
  Cmd_DirOpen,
  Cmd_DirClose,
  Cmd_DirRead,
}Command;

static void tcp_client(void)
{
    FILE *fp = NULL;
    DIR *dp = NULL;
    uint8_t rx_buffer[32];
    uint8_t tx_buffer[256];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");
	ESP_LOGI(TAG, "send hello");
	send(sock, hello, sizeof(hello), 0);

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
	    switch(rx_buffer[0]){
	    case Cmd_FileOpen:
	      fp = fopen((char *)&rx_buffer[1], "r");
	      tx_buffer[0] = 1;
	      tx_buffer[1] = (fp==NULL)?1:0;
	      break;
	    case Cmd_FileClose:
	      tx_buffer[0] = 1;
	      tx_buffer[1] = fclose(fp);
	      break;
	    case Cmd_FileRead:
	      size_t nr = fread(&tx_buffer[1], 1, rx_buffer[1], fp);
	      tx_buffer[0] = nr;
	      break;
	    case Cmd_DirOpen:
	      dp = opendir((char *)&rx_buffer[1]);
	      tx_buffer[0] = 1;
	      tx_buffer[1] = (dp==NULL)?1:0;
	      break;
	    case Cmd_DirClose:
	      tx_buffer[0] = 1;
	      tx_buffer[1] = closedir(dp);
	      dp = NULL;
	      break;
	    case Cmd_DirRead:
	      int nitem = rx_buffer[1];
	      uint8_t *p = &tx_buffer[1];
	      while(nitem--){
		struct dirent *dire = readdir(dp);
		if(dire == NULL){
		  *p++ = 0;
		  break;
		}
		strlcpy((char *)p, dire->d_name, 64);
		p += strlen(dire->d_name)+1;
		if(tx_buffer+sizeof(tx_buffer)-p < 64){
		  break;
		}
	      }
	      tx_buffer[0] = p - &tx_buffer[1];
	      break;
	    default:
	      tx_buffer[0] = 0;
	      break;
	    }
	    send(sock, tx_buffer, tx_buffer[0]+1, 0);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}

void client_fn(void)
{
  while(1){
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    tcp_client();
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}
