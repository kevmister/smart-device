/*
 * mdns.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_SERVICE_MDNS_H_
#define MAIN_SERVICES_SERVICE_MDNS_H_

#include "../services.h"

service_t service_mdns;

typedef struct  {
	char *instance_name;
	char *service_type;
	char *protocol;
	int port;
} service_mdns_config_service_t;

typedef union {
	service_config_t service_config;
	int port;
	char *hostname;
	char *default_instance_name;
	service_mdns_config_service_t *service_mdns_config_services;
	int number_services;
} service_mdns_config_t;

service_init_error_t service_mdns_init();
service_config_error_t service_mdns_config(void *service_config);
service_start_error_t service_mdns_start();
service_stop_error_t service_mdns_stop();
service_end_error_t service_mdns_end();

#endif /* MAIN_SERVICES_SERVICE_MDNS_H_ */
