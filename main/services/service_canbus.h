/*
 * mdns.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_SERVICE_CANBUS_H_
#define MAIN_SERVICES_SERVICE_CANBUS_H_

#include "../services.h"
#include "cJSON.h"

service_t service_canbus;

typedef struct  {
	char *instance_name;
	char *service_type;
	char *protocol;
	int port;
} service_canbus_config_service_t;

typedef union {
	service_config_t service_config;
	service_canbus_config_service_t *service_canbus_config_services;
	int number_services;
} service_canbus_config_t;

service_init_error_t service_canbus_init();
service_config_error_t service_canbus_config(cJSON *service_config);
service_start_error_t service_canbus_start();
service_stop_error_t service_canbus_stop();
service_end_error_t service_canbus_end();

#endif /* MAIN_SERVICES_SERVICE_CANBUS_H_ */
