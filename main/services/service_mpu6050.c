/*
 * service_mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_mpu6050.h"
#include <string.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <mpu6050/mpu6050.h>
#include <mathc.h>

#define TAG "SERVICE_MPU6050"

#define PI              3.14159265358979323846f
#define I2C_SDA         26
#define I2C_SCL         25
#define I2C_FREQ        100000
#define I2C_PORT        I2C_NUM_0

float self_test[6] = {0, 0, 0, 0, 0, 0};
float accel_bias[3] = {0, 0, 0};
float gyro_bias[3] = {0, 0, 0};

mpu6050_data_t mpu6050_data;

service_init_error_t service_mpu6050_init(){
	if(service_mpu6050.initialized){
		return SERVICE_INIT_FAILED;
	}

	i2c_config_t conf = {};
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_SDA;
	conf.scl_io_num = I2C_SCL;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_FREQ;
	i2c_param_config(I2C_PORT, &conf);
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

	ESP_LOGI(mpu6050_get_tag(), "Device ID: %d.", mpu6050_get_device_id());
	mpu6050_self_test(self_test);
	ESP_LOGI(mpu6050_get_tag(), "Device performing self-test");

	if (self_test[0] < 1.0f && self_test[1] < 1.0f && self_test[2] < 1.0f &&
		self_test[3] < 1.0f && self_test[4] < 1.0f && self_test[5] < 1.0f) {
	  //mpu6050_reset();
    ESP_LOGI(mpu6050_get_tag(), "Device passed self-test");
		//mpu6050_calibrate(accel_bias, gyro_bias);
		ESP_LOGI(mpu6050_get_tag(), "Device being calibrated");
		mpu6050_init();
		ESP_LOGI(mpu6050_get_tag(), "Device initialized");
		service_mpu6050.initialized = true;
		return SERVICE_INIT_OK;
	} else {
		ESP_LOGI(mpu6050_get_tag(), "Device did not pass self-test");
		return SERVICE_INIT_FAILED;
	}
}

service_config_error_t service_mpu6050_config(cJSON *service_config){
	service_mpu6050.configured = true;
	return SERVICE_CONFIG_OK;
}

void service_mpu6050_task(void *ignore){
	int16_t temp;

	float initial_pitch = 0, initial_roll = 0, pitch, roll;
	short calibrated = true;

	int samples = 128;
  mpu6050_acceleration_t sample_acceleration;
  mpu6050_rotation_t sample_rotation;
  vector_t sample_acceleration_final, sample_rotation_final;

	while(true){
		if (!mpu6050_get_int_dmp_status()) {

		  sample_acceleration_final = (vector_t){0,0,0};
		  sample_rotation_final = (vector_t){0,0,0};

		  for(int i=0; i<samples; i++){
		    mpu6050_get_acceleration(&sample_acceleration);
		    mpu6050_get_rotation(&sample_rotation);

        sample_acceleration_final.x += sample_acceleration.accel_x / samples;
        sample_acceleration_final.y += sample_acceleration.accel_y / samples;
        sample_acceleration_final.z += sample_acceleration.accel_z / samples;

        sample_rotation_final.x += sample_rotation.gyro_x / samples;
        sample_rotation_final.y += sample_rotation.gyro_y / samples;
        sample_rotation_final.z += sample_rotation.gyro_z / samples;

        vTaskDelay(2/portTICK_PERIOD_MS);

		  }

		  mpu6050_data.acceleration = sample_acceleration_final;
		  mpu6050_data.rotation = sample_rotation_final;

			temp = mpu6050_get_temperature();
			mpu6050_data.temperature = (float) temp / 340.0 + 36.53;

			pitch = 180 * atan2(mpu6050_data.acceleration.x, sqrt(mpu6050_data.acceleration.y*mpu6050_data.acceleration.y + mpu6050_data.acceleration.z*mpu6050_data.acceleration.z))/PI;
			roll = 180 * atan2(mpu6050_data.acceleration.y, sqrt(mpu6050_data.acceleration.x*mpu6050_data.acceleration.x + mpu6050_data.acceleration.z*mpu6050_data.acceleration.z))/PI;

			if(!calibrated){
			  initial_pitch = pitch;
			  initial_roll = roll;
			  calibrated = true;

		    ESP_LOGI(TAG, "Initial pitch %0.6f", pitch);
		    ESP_LOGI(TAG, "Initial roll %0.6f", roll);
			}

			mpu6050_data.pitch = pitch - initial_pitch;
			mpu6050_data.roll = roll - initial_roll;
		}
	}

	vTaskDelete(NULL);
}

service_start_error_t service_mpu6050_start(){
	xTaskCreate(&service_mpu6050_task, "service_mpu6050_task", 8192, NULL, 5, NULL);
	service_mpu6050.started = true;
	return SERVICE_START_OK;
}

service_stop_error_t service_mpu6050_stop(){
	service_mpu6050.started = false;
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
	service_mpu6050.initialized = false;
	service_mpu6050.configured = false;
	service_mpu6050.started = false;
	return SERVICE_END_OK;
}

mpu6050_data_t* get_mpu6050_data(){
	return &mpu6050_data;
};

service_mpu6050_interface_t service_mpu6050_interface = {
		.get_mpu6050_data = get_mpu6050_data
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

