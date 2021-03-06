/*
 * service_mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_mdns.h"
#include "esp_err.h"
#include "mdns.h"

service_init_error_t service_mdns_init(){
	if(service_mdns.initialized){
		return SERVICE_INIT_FAILED;
	}

	if(mdns_init() != ESP_OK){
		return SERVICE_INIT_FAILED;
	}

	service_mdns.initialized = 1;
	if(service_mdns.start_handler() != SERVICE_START_OK){
		return SERVICE_INIT_FAILED;
	}
	return SERVICE_INIT_OK;
}

service_config_error_t service_mdns_config(cJSON *service_config){
	cJSON *services_array;
	cJSON *service_array_item;
	mdns_hostname_set(cJSON_GetObjectItem(service_config, "hostname")->valuestring);
	mdns_instance_name_set(cJSON_GetObjectItem(service_config, "instance_name")->valuestring);
	services_array = cJSON_GetObjectItem(service_config, "mdns_services");
	service_array_item = services_array->child;
	while(service_array_item) {
		mdns_service_add(cJSON_GetObjectItem(service_array_item, "instance_name")->valuestring, cJSON_GetObjectItem(service_array_item, "service_type")->valuestring, cJSON_GetObjectItem(service_array_item, "protocol")->valuestring, cJSON_GetObjectItem(service_array_item, "port")->valueint, NULL, 0);
		service_array_item = service_array_item->next;
	}
//	for(int i=0; i <= cJSON_GetArraySize(); i++){
//		service_mdns_config_service_t *service_mdns_config_service = &service_config->service_mdns_config_services[i];
//		mdns_service_add(service_mdns_config_service->instance_name, service_mdns_config_service->service_type,	service_mdns_config_service->protocol, service_mdns_config_service->port, NULL, 0);
//	}
	return SERVICE_CONFIG_OK;
}

service_start_error_t service_mdns_start(){
	service_mdns.started = 1;
	return SERVICE_START_OK;
}

service_stop_error_t service_mdns_stop(){
	service_mdns.started = 0;
	if(service_mdns.end_handler() != SERVICE_END_OK){
		return SERVICE_STOP_FAILED;
	}
	return SERVICE_STOP_OK;
}

service_end_error_t service_mdns_end(){
	if(!service_mdns.initialized){
		return SERVICE_END_FAILED;
	}
	if(service_mdns.started){
		service_mdns.stop_handler();
		return SERVICE_END_FAILED;
	}
	mdns_free();
	service_mdns.initialized = 0;
	service_mdns.configured = 0;
	service_mdns.started = 0;
	return SERVICE_END_OK;
}

service_t service_mdns = {
		.name = "MDNS",
		.initialized = 0,
		.configured = 0,
		.started = 0,
		.init_handler = service_mdns_init,
		.config_handler = service_mdns_config,
		.start_handler = service_mdns_start,
		.stop_handler = service_mdns_stop,
		.end_handler = service_mdns_end
};

