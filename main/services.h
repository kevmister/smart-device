/*
 * services.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_SERVICES_SERVICES_H_
#define MAIN_SERVICES_SERVICES_H_

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
	int initialized;
	int configured;
	int started;
	service_init_error_t (*init_handler)();
	service_config_error_t (*config_handler)(void *service_config);
	service_start_error_t (*start_handler)();
	service_stop_error_t (*stop_handler)();
	service_end_error_t (*end_handler)();
} service_t;

#include "services/service_mdns.h"

#endif /* MAIN_SERVICES_SERVICES_H_ */
