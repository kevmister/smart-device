/*
 * program_mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "program_air_suspension.h"
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

program_init_error_t program_air_suspension_init(){
  if(program_air_suspension.initialized){
    return PROGRAM_INIT_FAILED;
  }
  program_air_suspension.initialized = true;
  return PROGRAM_INIT_OK;
}


program_config_error_t program_air_suspension_config(cJSON *program_config){
  program_air_suspension.configured = true;
  return PROGRAM_CONFIG_OK;
}

void program_air_suspension_task(void *ignore){
    vTaskDelete(NULL);
}

program_start_error_t program_air_suspension_start(){
  xTaskCreate(&program_air_suspension_task, "program_air_suspension_task", 8192, NULL, 5, NULL);
  program_air_suspension.started = true;
  return PROGRAM_START_OK;
}

program_stop_error_t program_air_suspension_stop(){
  program_air_suspension.started = false;
  if(program_air_suspension.end_handler() != PROGRAM_END_OK){
    return PROGRAM_STOP_FAILED;
  }
  return PROGRAM_STOP_OK;
}

program_end_error_t program_air_suspension_end(){
  if(!program_air_suspension.initialized){
    return PROGRAM_END_FAILED;
  }
  if(program_air_suspension.started){
    program_air_suspension.stop_handler();
    return PROGRAM_END_FAILED;
  }
  program_air_suspension.initialized = false;
  program_air_suspension.configured = false;
  program_air_suspension.started = false;
  return PROGRAM_END_OK;
}

program_air_suspension_interface_t program_air_suspension_interface = {};

program_t program_air_suspension = {
    .name = "AIR_SUSPENSION",
    .initialized = false,
    .configured = false,
    .started = false,
    .init_handler = program_air_suspension_init,
    .config_handler = program_air_suspension_config,
    .start_handler = program_air_suspension_start,
    .stop_handler = program_air_suspension_stop,
    .end_handler = program_air_suspension_end,
    .interface = &program_air_suspension_interface
};

