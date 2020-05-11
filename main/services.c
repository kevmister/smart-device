/*
 * services.c
 *
 *  Created on: May 10, 2020
 *      Author: kevmi
 */

#include "services.h"

service_t *services[3] = {
	&service_wifi,
	&service_http_server,
	&service_mdns
};

service_t *service_get_by_name(char *name){
   for(int i=0; i <= (sizeof(services)/sizeof(service_t *) - 1); i++){
	   service_t *service = (service_t *)services[i];
	   if(strcmp(name, service->name) == 0){
		   return service;
	   }
   }
   return NULL;
}
