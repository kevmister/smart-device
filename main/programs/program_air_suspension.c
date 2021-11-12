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
#include "../services.h"
#include "driver/gpio.h"
#include "string.h"

#define TAG "PROGRAM_AIR_SUSPENSION"

#define MESSAGE_ID_REQUEST_ALL            0x00
#define MESSAGE_ID_SENSOR_LEFT_PRESSURE   0x01
#define MESSAGE_ID_SENSOR_RIGHT_PRESSURE  0x02
#define MESSAGE_ID_VALUE_PITCH            0x03
#define MESSAGE_ID_VALUE_ROLL             0x04

#define GPIO_LEFT_RAISE     16
#define GPIO_LEFT_LOWER     17
#define GPIO_RIGHT_RAISE    33
#define GPIO_RIGHT_LOWER    18

#define MODE_RAISE          1
#define MODE_HOLD           0
#define MODE_LOWER         -1

program_init_error_t program_air_suspension_init () {
  if (program_air_suspension.initialized) {
    return PROGRAM_INIT_FAILED;
  }
  program_air_suspension.initialized = true;
  return PROGRAM_INIT_OK;
}

program_config_error_t program_air_suspension_config (cJSON *program_config) {
  program_air_suspension.configured = true;
  return PROGRAM_CONFIG_OK;
}

void program_air_suspension_task (void *ignore) {
  int LEFT_MODE = MODE_HOLD, RIGHT_MODE = MODE_HOLD;
  gpio_reset_pin(GPIO_LEFT_RAISE);
  gpio_reset_pin(GPIO_LEFT_LOWER);
  gpio_reset_pin(GPIO_RIGHT_RAISE);
  gpio_reset_pin(GPIO_RIGHT_LOWER);
  gpio_set_direction(GPIO_LEFT_RAISE, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_LEFT_LOWER, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_RIGHT_RAISE, GPIO_MODE_OUTPUT);
  gpio_set_direction(GPIO_RIGHT_LOWER, GPIO_MODE_OUTPUT);
  while (true) {
    service_mpu6050_interface_t *mpu6050 = service_mpu6050.interface;
    mpu6050_data_t *mpu6050_data = mpu6050->get_mpu6050_data ();
    ESP_LOGE(TAG, "roll: %.1f pitch: %.1f", mpu6050_data->roll, mpu6050_data->pitch);
    vTaskDelay (100 / portTICK_PERIOD_MS);

    if(mpu6050_data->roll > -1 && mpu6050_data->roll < 1 && mpu6050_data->pitch > -1 && mpu6050_data->pitch < 1){
      LEFT_MODE = MODE_HOLD;
      RIGHT_MODE = MODE_HOLD;
    }

    if(mpu6050_data->pitch > 1){
      LEFT_MODE = MODE_LOWER;
      RIGHT_MODE = MODE_LOWER;
    }

    if(mpu6050_data->pitch < -1){
      LEFT_MODE = MODE_RAISE;
      RIGHT_MODE = MODE_RAISE;
    }

    if(mpu6050_data->roll > 1){
      LEFT_MODE = MODE_LOWER;
      RIGHT_MODE = MODE_RAISE;
    }

    if(mpu6050_data->roll < -1){
      LEFT_MODE = MODE_RAISE;
      RIGHT_MODE = MODE_LOWER;
    }

    gpio_set_level(GPIO_LEFT_RAISE, LEFT_MODE == MODE_RAISE ? 1 : 0);
    gpio_set_level(GPIO_LEFT_LOWER, LEFT_MODE == MODE_LOWER ? 1 : 0);
    gpio_set_level(GPIO_RIGHT_RAISE, RIGHT_MODE == MODE_RAISE ? 1 : 0);
    gpio_set_level(GPIO_RIGHT_LOWER, RIGHT_MODE == MODE_LOWER ? 1 : 0);
  }
  vTaskDelete (NULL);
}

program_start_error_t program_air_suspension_start () {
  xTaskCreate (&program_air_suspension_task, "program_air_suspension_task", 8192, NULL, 5, NULL);
  program_air_suspension.started = true;
  return PROGRAM_START_OK;
}

program_stop_error_t program_air_suspension_stop () {
  program_air_suspension.started = false;
  if (program_air_suspension.end_handler () != PROGRAM_END_OK) {
    return PROGRAM_STOP_FAILED;
  }
  return PROGRAM_STOP_OK;
}

program_end_error_t program_air_suspension_end () {
  if (!program_air_suspension.initialized) {
    return PROGRAM_END_FAILED;
  }
  if (program_air_suspension.started) {
    program_air_suspension.stop_handler ();
    return PROGRAM_END_FAILED;
  }
  program_air_suspension.initialized = false;
  program_air_suspension.configured = false;
  program_air_suspension.started = false;
  return PROGRAM_END_OK;
}

program_air_suspension_interface_t program_air_suspension_interface = {};

void receive_data(program_data_t *program_data){
  service_mpu6050_interface_t *mpu6050 = service_mpu6050.interface;
  mpu6050_data_t *mpu6050_data = mpu6050->get_mpu6050_data();
  switch(program_data->message_id) {
    case MESSAGE_ID_REFRESH_OUTPUTS: {
      program_data_t program_data_send;
      program_data_send.message_id = MESSAGE_ID_VALUE_PITCH;
      memcpy(&program_data_send.message, &mpu6050_data->pitch, sizeof (mpu6050_data->pitch));
      program_send_data(&program_data_send);
      break;
    }
  }
}

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
    .receive_data = receive_data,
    .interface = &program_air_suspension_interface
};

