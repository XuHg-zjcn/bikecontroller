# 自行车控制器固件
本固件用于ESP32微控制器  
esp-idf含有未提供源代码的专有软件(Apache2许可证)，待逆向工程  

## 已实现功能
显示当前时速和里程
记录轮速计数据到FLASH中，通过WiFi读取
霍尔超时后，CPU进入Light-sleep模式，关闭显示屏背光省电
SD卡储存(暂时只有初始化程序)

## 目标功能
显示屏，按键
轮速计,GNSS,IMU,融合定位
记录和显示速度，轨迹，里程
曲柄功率计
电源控制(电池组,太阳能光伏,发电机,电源接口)
通信(WiFi,蓝牙,LoRa,红外)
指纹识别
车锁控制

## 版权与许可
Copyright (C) 2024 徐瑞骏(科技骏马)
本程序以GNU通用公共许可证第三版或更新(见COPYING文件)授权
部分代码可能有LGPLv3例外，具体请见各文件头部。
