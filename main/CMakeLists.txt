idf_component_register(SRCS "main.c"
	                    "wifi.c" "tcp_client_v4.c"
                            "led.c" "u8g2_user.c"
			    "wheelspeed.c"
			    "storage.c"
			    "power.c"
			    "sdcard.c"
			    "i2c.c"
			    "mpu9250.cpp"
			    "record.cpp"
                       INCLUDE_DIRS ".")

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)
else()
    fail_at_build_time(littlefs "Windows does not support LittleFS partition generation")
endif()
