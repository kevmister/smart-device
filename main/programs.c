/*
 * programs.c
 *
 *  Created on: May 10, 2020
 *      Author: kevmi
 */

#include "programs.h"
#include <string.h>

program_t *programs[10] = {
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
