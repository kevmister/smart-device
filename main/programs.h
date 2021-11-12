/*
 * programs.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_PROGRAMS_PROGRAMS_H_
#define MAIN_PROGRAMS_PROGRAMS_H_

#include "cJSON.h"
#include "main.h"
#include "services.h"

typedef enum {
	PROGRAM_STATE_NOT_INITIALIZED,
	PROGRAM_STATE_NOT_CONFIGURED,
	PROGRAM_STATE_NOT_STARTED,
	PROGRAM_STATE_STARTED
} program_state_t;

typedef enum {
	PROGRAM_INIT_OK,
	PROGRAM_INIT_FAILED
} program_init_error_t;

typedef enum {
	PROGRAM_CONFIG_OK,
	PROGRAM_CONFIG_FAILED
} program_config_error_t;

typedef enum {
	PROGRAM_START_OK,
	PROGRAM_START_FAILED
} program_start_error_t;

typedef enum {
	PROGRAM_STOP_OK,
	PROGRAM_STOP_FAILED
} program_stop_error_t;

typedef enum {
	PROGRAM_END_OK,
	PROGRAM_END_FAILED
} program_end_error_t;

typedef struct {
} program_config_t;

typedef struct {
	char *name;
	int initialized;
	int configured;
	int started;
	cJSON *program_config;
	program_init_error_t (*init_handler)();
	program_config_error_t (*config_handler)(cJSON *program_config);
	program_start_error_t (*start_handler)();
	program_stop_error_t (*stop_handler)();
	program_end_error_t (*end_handler)();
	void (*receive_data)(program_data_t *program_data);
	void *interface;
} program_t;

program_t *programs[1];

program_t *program_get_by_name(char *name);
void program_send_data(program_data_t *program_data);

#include "programs/program_air_suspension.h"

#endif /* MAIN_PROGRAMS_PROGRAMS_H_ */
