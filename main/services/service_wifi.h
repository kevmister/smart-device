/*
 * service_wifi.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_SERVICE_WIFI_H_
#define MAIN_SERVICES_SERVICE_WIFI_H_

#include "../services.h"
#include <stdio.h>
#include "esp_event.h"
#include "cJSON.h"

service_t service_wifi;

typedef struct  {
	int sta_enabled;
	int ap_enabled;
} service_wifi_config_service_t;

typedef union {
	service_config_t service_config;
} service_wifi_config_t;

void service_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

service_init_error_t service_wifi_init();
service_config_error_t service_wifi_config(cJSON *service_config);
service_start_error_t service_wifi_start();
service_stop_error_t service_wifi_stop();
service_end_error_t service_wifi_end();

#endif /* MAIN_SERVICES_SERVICE_WIFI_H_ */
