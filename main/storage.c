/*
 * 数据储存
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
#include "storage.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_littlefs.h"

#include <sys/types.h>
#include <dirent.h>

#define CNT_PER_FLUSH  32

static const char *TAG = "esp_littlefs";
static FILE *fp_rec = NULL;

void littlefs_init()
{
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

static int storage_get_next_filename(char *str_out)
{
    struct dirent *dire;
    int last = 0;
    DIR *dir = opendir("/littlefs");
    if(dir == NULL){
        return -1;
    }
    while((dire = readdir(dir)) != NULL){
	size_t len = strlen(dire->d_name);
	if(len>4 && strcmp(dire->d_name+(len-4), ".dat")==0){
	    int tmp = atoi(dire->d_name);
	    if(tmp > last){
	      last = tmp;
	    }
	}
    }
    sprintf(str_out, "/littlefs/%d.dat", last+1);
    return 0;
}

void storage_record_wheelspeed(uint32_t hall_cnt, uint64_t ticks_hall)
{
  if (ticks_hall > UINT32_MAX){
    if(fp_rec != NULL){
      fclose(fp_rec);
      fp_rec = NULL;
    }
  }
  if(ticks_hall <= UINT32_MAX){
    uint32_t ticks_hall_u32 = ticks_hall;
    if(fp_rec == NULL){
      char fname[16];
      ESP_LOGI(TAG, "Getting filename");
      storage_get_next_filename(fname);
      ESP_LOGI(TAG, "Opening file %s", fname);
      fp_rec = fopen(fname, "w");
      if(fp_rec == NULL){
	ESP_LOGE(TAG, "Open Failed");
      }
    }
    if(fp_rec != NULL){
      fwrite(&ticks_hall_u32, sizeof(uint32_t), 1, fp_rec);
      if(hall_cnt % CNT_PER_FLUSH == 0){
	ESP_LOGI(TAG, "Flush data");
	if(fflush(fp_rec) != 0){
	  ESP_LOGE(TAG, "Flush Failed");
	}
      }
    }
  }
}
