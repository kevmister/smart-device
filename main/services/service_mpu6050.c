/*
 * service_mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_mpu6050.h"
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#define I2C_ADDRESS 0x68 // I2C address of MPU6050
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_PWR_MGMT_1   0x6B

gpio_num_t PIN_SDA;
gpio_num_t PIN_CLK;

short accel_x;
short accel_y;
short accel_z;

service_init_error_t service_mpu6050_init(){
	if(service_mpu6050.initialized){
		return SERVICE_INIT_FAILED;
	}
	service_mpu6050.initialized = true;
	return SERVICE_INIT_OK;
}


service_config_error_t service_mpu6050_config(cJSON *service_config){
	PIN_SDA = (gpio_num_t)cJSON_GetObjectItem(service_config, "pin_sda")->valueint;
	PIN_CLK = (gpio_num_t)cJSON_GetObjectItem(service_config, "pin_clk")->valueint;

	service_mpu6050.configured = 1;
	return SERVICE_CONFIG_OK;
}

void service_mpu6050_task(void *ignore){
	i2c_config_t conf;
		conf.mode = I2C_MODE_MASTER;
		conf.sda_io_num = PIN_SDA;
		conf.scl_io_num = PIN_CLK;
		conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
		conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
		conf.master.clk_speed = 100000;
		ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
		ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

		i2c_cmd_handle_t cmd;
		vTaskDelay(200/portTICK_PERIOD_MS);

		cmd = i2c_cmd_link_create();
		ESP_ERROR_CHECK(i2c_master_start(cmd));
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
		i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, 1);
		ESP_ERROR_CHECK(i2c_master_stop(cmd));
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);

		cmd = i2c_cmd_link_create();
		ESP_ERROR_CHECK(i2c_master_start(cmd));
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
		i2c_master_write_byte(cmd, MPU6050_PWR_MGMT_1, 1);
		i2c_master_write_byte(cmd, 0, 1);
		ESP_ERROR_CHECK(i2c_master_stop(cmd));
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);


		uint8_t data[14];
		while(1) {
			// Tell the MPU6050 to position the internal register pointer to register
			// MPU6050_ACCEL_XOUT_H.
			cmd = i2c_cmd_link_create();
			ESP_ERROR_CHECK(i2c_master_start(cmd));
			ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
			ESP_ERROR_CHECK(i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, 1));
			ESP_ERROR_CHECK(i2c_master_stop(cmd));
			ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
			i2c_cmd_link_delete(cmd);

			cmd = i2c_cmd_link_create();
			ESP_ERROR_CHECK(i2c_master_start(cmd));
			ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_READ, 1));

			ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data,   0));
			ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+1, 0));
			ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+2, 0));
			ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+3, 0));
			ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+4, 0));
			ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+5, 1));

			//i2c_master_read(cmd, data, sizeof(data), 1);
			ESP_ERROR_CHECK(i2c_master_stop(cmd));
			ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
			i2c_cmd_link_delete(cmd);

			accel_x = (data[0] << 8) | data[1];
			accel_y = (data[2] << 8) | data[3];
			accel_z = (data[4] << 8) | data[5];
			ESP_LOGD("MPU6050 SERVICE", "accel_x: %d, accel_y: %d, accel_z: %d", accel_x, accel_y, accel_z);

			vTaskDelay(500/portTICK_PERIOD_MS);
		}

		vTaskDelete(NULL);
}

service_start_error_t service_mpu6050_start(){
	xTaskCreate(&service_mpu6050_task, "service_mpu6050_task", 8192, NULL, 5, NULL);
	service_mpu6050.started = 1;
	return SERVICE_START_OK;
}

service_stop_error_t service_mpu6050_stop(){
	service_mpu6050.started = 0;
	if(service_mpu6050.end_handler() != SERVICE_END_OK){
		return SERVICE_STOP_FAILED;
	}
	return SERVICE_STOP_OK;
}

service_end_error_t service_mpu6050_end(){
	if(!service_mpu6050.initialized){
		return SERVICE_END_FAILED;
	}
	if(service_mpu6050.started){
		service_mpu6050.stop_handler();
		return SERVICE_END_FAILED;
	}
	service_mpu6050.initialized = 0;
	service_mpu6050.configured = 0;
	service_mpu6050.started = 0;
	return SERVICE_END_OK;
}

int read_temperature(){
	return 10;
}

service_mpu6050_interface_t service_mpu6050_interface = {
	.read_temperature = read_temperature
};

service_t service_mpu6050 = {
		.name = "MPU6050",
		.initialized = 0,
		.configured = 0,
		.started = 0,
		.init_handler = service_mpu6050_init,
		.config_handler = service_mpu6050_config,
		.start_handler = service_mpu6050_start,
		.stop_handler = service_mpu6050_stop,
		.end_handler = service_mpu6050_end,
		.interface = &service_mpu6050_interface
};

