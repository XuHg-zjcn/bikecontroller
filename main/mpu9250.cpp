/*
 * MPU9250驱动程序
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
#include "mpu9250.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "buffer.hpp"
#include "writer.hpp"

//register names copy from document "RM-MPU-9250A-00", revision 1.4
#define SELF_TEST_X_GYRO     0x00
#define SELF_TEST_Y_GYRO     0x01
#define SELF_TEST_Z_GYRO     0x02
#define SELF_TEST_X_ACCEL    0x0D
#define SELF_TEST_Y_ACCEL    0x0E
#define SELF_TEST_Z_ACCEL    0x0F
#define XG_OFFSET_H          0x13
#define XG_OFFSET_L          0x14
#define YG_OFFSET_H          0x15
#define YG_OFFSET_L          0x16
#define ZG_OFFSET_H          0x17
#define ZG_OFFSET_L          0x18
#define SMPLRT_DIV           0x19
#define CONFIG               0x1A
#define GRYO_CONFIG          0x1B
#define ACCEL_CONFIG         0x1C
#define ACCEL_CONFIG2        0x1D
#define LP_ACCEL_ODR         0x1E
#define WOM_THR              0x1F
#define FIFO_EN              0x23
#define I2C_MST_CTRL         0x24
#define I2C_SLV0_ADDR        0x25
#define I2C_SLV0_REG         0x26
#define I2C_SLV0_CTRL        0x27
#define I2C_SLV1_ADDR        0x28
#define I2C_SLV1_REG         0x29
#define I2C_SLV1_CTRL        0x2A
#define I2C_SLV2_ADDR        0x2B
#define I2C_SLV2_REG         0x2C
#define I2C_SLV2_CTRL        0x2D
#define I2C_SLV3_ADDR        0x2E
#define I2C_SLV3_REG         0x2F
#define I2C_SLV3_CTRL        0x30
#define I2C_SLV4_ADDR        0x31
#define I2C_SLV4_REG         0x32
#define I2C_SLV4_DO          0x33
#define I2C_SLV4_CTRL        0x34
#define I2C_SLV4_DI          0x35
#define I2C_MST_STATUS       0x36
#define INT_PIN_CFG          0x37
#define INT_ENABLE           0x38
#define INT_STATUS           0x3A
#define ACCEL_XOUT_H         0x3B
#define ACCEL_XOUT_L         0x3C
#define ACCEL_YOUT_H         0x3D
#define ACCEL_YOUT_L         0x3E
#define ACCEL_ZOUT_H         0x3F
#define ACCEL_ZOUT_L         0x40
#define TEMP_OUT_H           0x41
#define TEMP_OUT_L           0x42
#define GYRO_XOUT_H          0x43
#define GYRO_XOUT_L          0x44
#define GYRO_YOUT_H          0x45
#define GYRO_YOUT_L          0x46
#define GYRO_ZOUT_H          0x47
#define GYRO_ZOUT_L          0x48
#define EXT_SENS_DATA_(x)    (x+0x49)
#define I2C_SLV0_DO          0x63
#define I2C_SLV1_DO          0x64
#define I2C_SLV2_DO          0x65
#define I2C_SLV3_DO          0x66
#define I2C_MST_DELAY_CTRL   0x67
#define SIGNAL_PATH_RESET    0x68
#define MOT_DETECT_CTRL      0x69
#define USER_CTRL            0x6A
#define PWR_MGMT_1           0x6B
#define PWR_MGMT_2           0x6C
#define FIFO_COUNTH          0x72
#define FIFO_COUNTL          0x73
#define FIFO_R_W             0x74
#define WHO_AM_I             0x75
#define XA_OFFSET_H          0x77
#define XA_OFFSET_L          0x78
#define YA_OFFSET_H          0x7A
#define YA_OFFSET_L          0x7B
#define ZA_OFFSET_H          0x7D
#define ZA_OFFSET_L          0x7E

#define MPU9250_I2C_ADDR     0x68
#define MPU9250_I2C_NUM      ((i2c_port_t)0)
#define MPU9250_I2C_TIMEOUT  100
#define AK8963_I2C_ADDR     0x0C

int16_t mpu9250_data[9];
Buffer buffmpu(2*9, 128);

//copy from esp-idf/examples/peripherals/i2c/i2c_simple/main/i2c_simple_main.c
/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
static esp_err_t mpu9250_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(MPU9250_I2C_NUM, MPU9250_I2C_ADDR, &reg_addr, 1, data, len, MPU9250_I2C_TIMEOUT / portTICK_PERIOD_MS);
}

static int mpu9250_register_read_byte(uint8_t reg_addr)
{
    uint8_t data;
    esp_err_t res = mpu9250_register_read(reg_addr, &data, 1);
    if(res != ESP_OK){
        return -1;
    }else{
        return data;
    }
}

/**
 * @brief Write a byte to a MPU9250 sensor register
 */
static esp_err_t mpu9250_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(MPU9250_I2C_NUM, MPU9250_I2C_ADDR, write_buf, sizeof(write_buf), MPU9250_I2C_TIMEOUT / portTICK_PERIOD_MS);

    return ret;
}

static int ak8963_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(MPU9250_I2C_NUM, AK8963_I2C_ADDR, &reg_addr, 1, data, len, MPU9250_I2C_TIMEOUT / portTICK_PERIOD_MS);
}

static int ak8963_register_read_byte(uint8_t reg_addr)
{
    uint8_t data;
    esp_err_t res = ak8963_register_read(reg_addr, &data, 1);
    if(res != ESP_OK){
        return -1;
    }else{
        return data;
    }
}

static esp_err_t ak8963_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(MPU9250_I2C_NUM, AK8963_I2C_ADDR, write_buf, sizeof(write_buf), MPU9250_I2C_TIMEOUT / portTICK_PERIOD_MS);

    return ret;
}

//MPU9250 I2C 主机读取函数暂时无法使用
static int mpu9250_slave_write_reg(uint8_t slave_addr, uint8_t reg_addr, uint8_t data)
{
   int mst_stat;
   int count=0;
   mpu9250_register_write_byte(I2C_SLV4_CTRL, 0x00); //disable slave4
   mpu9250_register_read_byte(I2C_MST_STATUS);       //clear flags
   mpu9250_register_write_byte(I2C_SLV4_ADDR,  slave_addr | (0U<<7));
   mpu9250_register_write_byte(I2C_SLV4_REG, reg_addr);
   mpu9250_register_write_byte(I2C_SLV4_DO, data);
   mpu9250_register_write_byte(I2C_SLV4_CTRL, (1U<<7));
   do{
     vTaskDelay(pdMS_TO_TICKS(10));
     mst_stat = mpu9250_register_read_byte(I2C_MST_STATUS);
   }while(!(mst_stat & 0x50) && (++count)<20);
   if(mst_stat & 0x10){
     return -1;
   }else if(count>=20){
     return -2;
   }else{
     return 0;
   }
}

static int mpu9250_slave_read_reg(uint8_t slave_addr, uint8_t reg_addr)
{
  int mst_stat;
  int count=0;
  mpu9250_register_write_byte(I2C_SLV4_CTRL, 0x00); //disable slave4
  mpu9250_register_read_byte(I2C_MST_STATUS);       //clear flags
  mpu9250_register_write_byte(I2C_SLV4_ADDR,  slave_addr | (1U<<7));
  mpu9250_register_write_byte(I2C_SLV4_REG, reg_addr);
  mpu9250_register_write_byte(I2C_SLV4_CTRL, (1U<<7));
  do{
    vTaskDelay(pdMS_TO_TICKS(10));
    mst_stat = mpu9250_register_read_byte(I2C_MST_STATUS);
  }while(!(mst_stat & 0x50) && (++count)<20);
  if(mst_stat & 0x10){
    return -1;
  }else if(count>=20){
    return -2;
  }else{
    return mpu9250_register_read_byte(I2C_SLV4_DI);
  }
}

int mpu9250_init()
{
  int i;
  uint8_t byte;
  //try 10 times
  for(i=0;i<10;i++){
    byte = 0;
    mpu9250_register_read(WHO_AM_I, &byte, 1);
    if(byte == 0x71){
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  if(i == 10){
    //failed
    return -1;
  }
  mpu9250_register_write_byte(PWR_MGMT_1, 0x80); //reset MPU9250
  mpu9250_register_write_byte(PWR_MGMT_1, 0x00); //exit sleep mode
  //mpu9250_register_write_byte(PWR_MGMT_2, 0x00); //enable all acc,gyro axes
  //mpu9250_register_write_byte(USER_CTRL, 0x20); //enable I2C Master
  mpu9250_register_write_byte(INT_PIN_CFG, 0x02); //enable bypass
  mpu9250_register_write_byte(GRYO_CONFIG, 0x18); //+-2000dps
  mpu9250_register_write_byte(ACCEL_CONFIG, 0x18); //+-16g
  //TODO: self-test
  //TODO: record magnetometer data
  //TODO: enable Wake-on-motion
  printf("gryo and accel config finish\n");

  int wia = ak8963_register_read_byte(0x00);
  //int wia = mpu9250_slave_read_reg(AK8963_I2C_ADDR, 0x00);
  if(wia < 0){
    printf("read mpu9250's magnet(AK8963) WIA failed %d\n", wia);
  }else if(wia != 0x48){
    printf("mpu9250's magnet WIA mistake 0x%02x\n", wia);
  }else{
    printf("mpu9250's magnet detected\n");
  }
  ak8963_register_write_byte(0x0B, 0x01); //reset
  vTaskDelay(pdMS_TO_TICKS(100));
  ak8963_register_write_byte(0x0A, 0x0f); //fuse ROM access
  uint8_t sen[3];
  ak8963_register_read(0x10, sen, 3);
  printf("ak8963 rom %d,%d,%d", sen[0], sen[1], sen[2]);
  ak8963_register_write_byte(0x0A, 0x16); //16bit output, 100Hz measure
  return 0;
}

//TODO: move C++ code to another file
void mpu9250_print_data()
{
  uint8_t buff[19];
  uint32_t count=0;
  while(1){
    vTaskDelay(pdMS_TO_TICKS(10));
    mpu9250_register_read(ACCEL_XOUT_H, buff, 6);
    mpu9250_register_read(GYRO_XOUT_H, buff+6, 6);
    for(int i=0;i<6;i++){
      mpu9250_data[i] = (buff[2*i]<<8) | buff[2*i+1];
    }
    if(ak8963_register_read_byte(0x02) & 0x01){
      ak8963_register_read(0x03, buff+12, 7); //require read ST2(else data are keep)
      for(int i=6;i<9;i++){
        mpu9250_data[i] = buff[2*i] | (buff[2*i+1]<<8);
      }
    }
    count++;
    if(count % 100 == 0){
      for(int i=0;i<9;i+=3){
        printf("% 5d,% 5d,% 5d   ", mpu9250_data[i+0], mpu9250_data[i+1], mpu9250_data[i+2]);
      }
      printf("\n");
    }
    buffmpu.w_head.push_force(1, mpu9250_data);
  }
}
