/*
 * mdns.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_SERVICE_SSD1315_H_
#define MAIN_SERVICES_SERVICE_SSD1315_H_

#include "../services.h"
#include "cJSON.h"

service_t service_ssd1315;

typedef struct  {
	char *instance_name;
	char *service_type;
	char *protocol;
	int port;
} service_ssd1315_config_service_t;

typedef union {
	service_config_t service_config;
	service_ssd1315_config_service_t *service_ssd1315_config_services;
	int number_services;
} service_ssd1315_config_t;

typedef struct {
} service_ssd1315_interface_t;

service_init_error_t service_ssd1315_init();
service_config_error_t service_ssd1315_config(cJSON *service_config);
service_start_error_t service_ssd1315_start();
service_stop_error_t service_ssd1315_stop();
service_end_error_t service_ssd1315_end();

#endif /* MAIN_SERVICES_SERVICE_SSD1315_H_ */
