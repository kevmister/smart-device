/*
 * mdns.h
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#ifndef MAIN_PROGRAMS_PROGRAM_AIR_SUSPENSION_H_
#define MAIN_PROGRAMS_PROGRAM_AIR_SUSPENSION_H_

#include "../programs.h"
#include "cJSON.h"

#define MESSAGE_ID_REFRESH_OUTPUTS      0x00

program_t program_air_suspension;

typedef struct  {
  char *instance_name;
  char *program_type;
  char *protocol;
  int port;
} program_air_suspension_config_program_t;

typedef union {
  program_config_t program_config;
  program_air_suspension_config_program_t *program_air_suspension_config_programs;
  int number_programs;
} program_air_suspension_config_t;

typedef struct {
} program_air_suspension_interface_t;

program_init_error_t program_air_suspension_init();
program_config_error_t program_air_suspension_config(cJSON *program_config);
program_start_error_t program_air_suspension_start();
program_stop_error_t program_air_suspension_stop();
program_end_error_t program_air_suspension_end();

#endif /* MAIN_PROGRAMS_PROGRAM_AIR_SUSPENSION_H_ */
