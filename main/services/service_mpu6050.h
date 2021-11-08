/*
 * mdns.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_SERVICE_MPU6050_H_
#define MAIN_SERVICES_SERVICE_MPU6050_H_

#include "../services.h"
#include "cJSON.h"
#include <mpu6050/mpu6050.h>

service_t service_mpu6050;

typedef struct  {
	char *instance_name;
	char *service_type;
	char *protocol;
	int port;
} service_mpu6050_config_service_t;

typedef union {
	service_config_t service_config;
	service_mpu6050_config_service_t *service_mpu6050_config_services;
	int number_services;
} service_mpu6050_config_t;

typedef struct {
	float x, y, z;
} vector_t;

typedef struct {
	int pitch, roll, yaw;
} rotation_t;

typedef struct {
	mpu6050_acceleration_t acceleration;
	mpu6050_rotation_t rotation;
	float temperature;
} mpu6050_data_t;

typedef struct {
	mpu6050_data_t* (*get_mpu6050_data)();
} service_mpu6050_interface_t;

service_init_error_t service_mpu6050_init();
service_config_error_t service_mpu6050_config(cJSON *service_config);
service_start_error_t service_mpu6050_start();
service_stop_error_t service_mpu6050_stop();
service_end_error_t service_mpu6050_end();

#endif /* MAIN_SERVICES_SERVICE_MPU6050_H_ */
