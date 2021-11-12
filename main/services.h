/*
 * services.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */


#ifndef MAIN_SERVICES_SERVICES_H_
#define MAIN_SERVICES_SERVICES_H_

#include "cJSON.h"
#include "main.h"
#include "programs.h"

typedef enum {
	SERVICE_STATE_NOT_INITIALIZED,
	SERVICE_STATE_NOT_CONFIGURED,
	SERVICE_STATE_NOT_STARTED,
	SERVICE_STATE_STARTED
} service_state_t;

typedef enum {
	SERVICE_INIT_OK,
	SERVICE_INIT_FAILED
} service_init_error_t;

typedef enum {
	SERVICE_CONFIG_OK,
	SERVICE_CONFIG_FAILED
} service_config_error_t;

typedef enum {
	SERVICE_START_OK,
	SERVICE_START_FAILED
} service_start_error_t;

typedef enum {
	SERVICE_STOP_OK,
	SERVICE_STOP_FAILED
} service_stop_error_t;

typedef enum {
	SERVICE_END_OK,
	SERVICE_END_FAILED
} service_end_error_t;

typedef struct {
} service_config_t;

typedef struct {
	char *name;
	int initialized;
	int configured;
	int started;
	cJSON *service_config;
	service_init_error_t (*init_handler)();
	service_config_error_t (*config_handler)(cJSON *service_config);
	service_start_error_t (*start_handler)();
	service_stop_error_t (*stop_handler)();
	service_end_error_t (*end_handler)();
	void (*send_data)(program_data_t *program_data);
	void *interface;
} service_t;

service_t *services[6];

service_t *service_get_by_name(char *name);

#include "services/service_mdns.h"
#include "services/service_wifi.h"
#include "services/service_http_server.h"
#include "services/service_canbus.h"
#include "services/service_mpu6050.h"
#include "services/service_ssd1315.h"

#endif /* MAIN_SERVICES_SERVICES_H_ */
