/*
 * service_mdns.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_ssd1315.h"
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "SERVICE_SSD1315"

#define I2C_SDA         26
#define I2C_SCL         25
#define I2C_FREQ        100000
#define I2C_PORT        I2C_NUM_0

const uint8_t ssd1306_init_sequence [] = {	// Initialization Sequence
	0xAE,			// Set Display ON/OFF - AE=OFF, AF=ON
	0xD5, 0xF0,		// Set display clock divide ratio/oscillator frequency, set divide ratio
	0xA8, 0x3F,		// Set multiplex ratio (1 to 64) ... (height - 1)
	0xD3, 0x00,		// Set display offset. 00 = no offset
	0x40 | 0x00,	// Set start line address, at 0.
	0x8D, 0x14,		// Charge Pump Setting, 14h = Enable Charge Pump
	0x20, 0x00,		// Set Memory Addressing Mode - 00=Horizontal, 01=Vertical, 10=Page, 11=Invalid
	0xA0 | 0x01,	// Set Segment Re-map
	0xC8,			// Set COM Output Scan Direction
	0xDA, 0x12,		// Set COM Pins Hardware Configuration - 128x32:0x02, 128x64:0x12
	0x81, 0x3F,		// Set contrast control register
	0xD9, 0x22,		// Set pre-charge period (0x22 or 0xF1)
	0xDB, 0x20,		// Set Vcomh Deselect Level - 0x00: 0.65 x VCC, 0x20: 0.77 x VCC (RESET), 0x30: 0.83 x VCC
	0xA4,			// Entire Display ON (resume) - output RAM to display
	0xA6,			// Set Normal/Inverse Display mode. A6=Normal; A7=Inverse
	0x2E,			// Deactivate Scroll command
	0xAF,			// Set Display ON/OFF - AE=OFF, AF=ON
};

#define SSD1306_SA		0X3C	// Slave address

#define SSD1306_COMMAND 0x00
#define SSD1306_DATA 0x40

void ssd1315_send_command_start(){
	I2CStop();
	I2CStart(SSD1306_SA, 0);
	I2CWrite(SSD1306_COMMAND);
}

void ssd1315_send_command_stop() {
	I2CStop();
}

void ssd1315_send_data_start(void) {
	I2CStop();
	I2CStart(SSD1306_SA, 0);
	I2CWrite(SSD1306_DATA);
}

void ssd1315_send_data_stop() {
	I2CStop();
}

void ssd1315_send_byte(uint8_t byte) {
	I2CWrite(byte);
}

void ssd1315_setpos(uint8_t x, uint8_t y)
{
	ssd1315_send_command_start();
	ssd1315_send_byte(0xb0 | (y & 0x07));
	ssd1315_send_byte(0x10 | ((x & 0xf0) >> 4));
	ssd1315_send_byte(x & 0x0f); // | 0x01
	ssd1315_send_command_stop();
}

void ssd1315_fillscreen(uint8_t fill) {
	ssd1315_setpos(0, 0);
	ssd1315_send_data_start();	// Initiate transmission of data
	for (uint16_t i = 0; i < 128 * 8 / 4; i++) {
		ssd1315_send_byte(fill);
		ssd1315_send_byte(fill);
		ssd1315_send_byte(fill);
		ssd1315_send_byte(fill);
	}
	ssd1315_send_data_stop();	// Finish transmission
}

void ssd1315_draw_bmp(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t bitmap[]){
	uint16_t j = 0;
	uint8_t y, x;
	if (y1 % 8 == 0) y = y1 / 8;
	else y = y1 / 8 + 1;
	for (y = y0; y < y1; y++)
	{
		ssd1315_setpos(x0,y);
		ssd1315_send_data_start();
		for (x = x0; x < x1; x++)
		{
			ssd1315_send_byte(pgm_read_byte(&bitmap[j++]));
		}
		ssd1315_send_data_stop();
	}
}

service_init_error_t service_ssd1315_init(){
	if(service_ssd1315.initialized){
		return SERVICE_INIT_FAILED;
	}

	i2c_config_t conf = {};
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_SDA;
	conf.scl_io_num = I2C_SCL;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_FREQ;
	i2c_param_config(I2C_PORT, &conf);

	ssd1315_send_command_start();
	for (uint8_t i = 0; i < sizeof (ssd1306_init_sequence); i++) {
		ssd1315_send_byte(pgm_read_byte(&ssd1306_init_sequence[i]));
	}
	ssd1315_send_command_stop();
	ssd1315_fillscreen(0);

	service_ssd1315.initialized = true;
	return SERVICE_INIT_OK;
}

service_config_error_t service_ssd1315_config(cJSON *service_config){
	service_ssd1315.configured = true;
	return SERVICE_CONFIG_OK;
}

void service_ssd1315_task(void *ignore){
		vTaskDelete(NULL);
}

service_start_error_t service_ssd1315_start(){
	xTaskCreate(&service_ssd1315_task, "service_ssd1315_task", 8192, NULL, 5, NULL);
	service_ssd1315.started = true;
	return SERVICE_START_OK;
}

service_stop_error_t service_ssd1315_stop(){
	service_ssd1315.started = false;
	if(service_ssd1315.end_handler() != SERVICE_END_OK){
		return SERVICE_STOP_FAILED;
	}
	return SERVICE_STOP_OK;
}

service_end_error_t service_ssd1315_end(){
	if(!service_ssd1315.initialized){
		return SERVICE_END_FAILED;
	}
	if(service_ssd1315.started){
		service_ssd1315.stop_handler();
		return SERVICE_END_FAILED;
	}
	service_ssd1315.initialized = false;
	service_ssd1315.configured = false;
	service_ssd1315.started = false;
	return SERVICE_END_OK;
}

service_ssd1315_interface_t service_ssd1315_interface = {
};

service_t service_ssd1315 = {
		.name = "SSD1315",
		.initialized = 0,
		.configured = 0,
		.started = 0,
		.init_handler = service_ssd1315_init,
		.config_handler = service_ssd1315_config,
		.start_handler = service_ssd1315_start,
		.stop_handler = service_ssd1315_stop,
		.end_handler = service_ssd1315_end,
		.interface = &service_ssd1315_interface
};

