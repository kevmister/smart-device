/*
 * mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "mdns.h"

service_init_error_t service_mdns_init(){
	if(service_mdns.initialized){
		return SERVICE_INIT_FAILED;
	}

	if(mdns_init() != ESP_OK){
		return SERVICE_INIT_FAILED;
	}

	service_mdns.initialized = true;
	if(service_mdns.service_start_handler() != SERVICE_START_OK){
		return SERVICE_INIT_FAILED;
	}
	return SERVICE_INIT_OK;
}

service_config_error_t service_mdns_config(service_mdns_config_t *service_config){
	mdns_hostname_set(service_config->hostname);
	mdns_instance_name_set(service_config->default_instance_name);
	for(int i=0; i <= sizeof(service_config->service_mdns_config_services) / sizeof(service_mdns_config_service_t); i++){
		service_mdns_config_service_t *service_mdns_config_service = service_config->service_mdns_config_services[i];
		mdns_service_add(service_mdns_config_service->instance_name, service_mdns_config_service->service_type,	service_mdns_config_service->protocol, service_mdns_config_service->port, NULL, 0);
	}
	return SERVICE_CONFIG_OK;
}

service_start_error_t service_mdns_start(){
	service_mdns.started = true;
	return SERVICE_START_OK;
}

service_stop_error_t service_mdns_stop(){
	service_mdns.started = false;
	if(service_mdns->service_end_handler() != SERVICE_END_OK){
		return SERVICE_STOP_FAILED;
	}
	return SERVICE_STOP_OK;
}

service_end_error_t service_mdns_end(){
	if(!service_mdns.initialized){
		return SERVICE_END_FAILED;
	}
	if(service_mdns.started){
		service_mdns->service_stop_handler();
		return SERVICE_END_FAILED;
	}
	mdns_free();
	service_mdns.initialized = false;
	service_mdns.configured = false;
	service_mdns.started = false;
	return SERVICE_END_OK;
}

service_t service_mdns = {
		.initialized = false,
		.configured = false,
		.started = false,
		.service_init_handler = *service_mdns_init,
		.service_config_handler = *service_mdns_config,
		.service_start_handler = *service_mdns_start,
		.service_stop_handler = *service_mdns_stop,
		.service_end_handler = *service_mdns_end
};
