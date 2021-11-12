/*
 * programs.c
 *
 *  Created on: May 10, 2020
 *      Author: kevmi
 */

#include "services.h"
#include <string.h>

program_t *programs[1] = {
	&program_air_suspension
};

program_t *program_get_by_name(char *name){
   for(int i=0; i <= (sizeof(programs)/sizeof(program_t *) - 1); i++){
	   program_t *program = (program_t *)programs[i];
	   if(strcmp(name, program->name) == 0){
		   return program;
	   }
   }
   return NULL;
}


void program_send_data(program_data_t *program_data){
  program_data_t program_data_copy;
  memcpy(&program_data_copy, &program_data, sizeof(program_data));
   for(int i=0; i <= (sizeof(services)/sizeof(service_t *) - 1); i++){
      service_t *service = (service_t *)services[i];
      service->send_data(&program_data_copy);
   }
}
void program_receive_data(program_data_t *program_data){
  program_data_t program_data_copy;
  memcpy(&program_data_copy, &program_data, sizeof(program_data));
   for(int i=0; i <= (sizeof(programs)/sizeof(program_t *) - 1); i++){
      program_t *program = (program_t *)programs[i];
      program->receive_data(&program_data_copy);
   }
}
