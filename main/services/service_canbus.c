/*
 * service_mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_canbus.h"
#include "esp_err.h"

service_init_error_t service_canbus_init(){
	if(service_canbus.initialized){
		return SERVICE_INIT_FAILED;
	}

	service_canbus.initialized = 1;
	if(service_canbus.start_handler() != SERVICE_START_OK){
		return SERVICE_INIT_FAILED;
	}
	return SERVICE_INIT_OK;
}

service_config_error_t service_canbus_config(cJSON *service_config){
	return SERVICE_CONFIG_OK;
}

service_start_error_t service_canbus_start(){
	service_canbus.started = 1;
	return SERVICE_START_OK;
}

service_stop_error_t service_canbus_stop(){
	service_canbus.started = 0;
	if(service_canbus.end_handler() != SERVICE_END_OK){
		return SERVICE_STOP_FAILED;
	}
	return SERVICE_STOP_OK;
}

service_end_error_t service_canbus_end(){
	if(!service_canbus.initialized){
		return SERVICE_END_FAILED;
	}
	if(service_canbus.started){
		service_canbus.stop_handler();
		return SERVICE_END_FAILED;
	}
	service_canbus.initialized = 0;
	service_canbus.configured = 0;
	service_canbus.started = 0;
	return SERVICE_END_OK;
}

service_t service_canbus = {
		.name = "CANBUS",
		.initialized = 0,
		.configured = 0,
		.started = 0,
		.init_handler = service_canbus_init,
		.config_handler = service_canbus_config,
		.start_handler = service_canbus_start,
		.stop_handler = service_canbus_stop,
		.end_handler = service_canbus_end
};

