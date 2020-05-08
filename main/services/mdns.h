/*
 * mdns.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_MDNS_H_
#define MAIN_SERVICES_MDNS_H_

service_t service_mdns;

typedef struct  {
	char *instance_name;
	char *service_type;
	char *protocol;
	int port;
} service_mdns_config_service_t;

typedef union {
	service_config_t service_config;
	int port = 5353;
	char *hostname;
	char *default_instance_name;
	service_mdns_config_service_t *service_mdns_config_services;
} service_mdns_config_t;

#endif /* MAIN_SERVICES_MDNS_H_ */
