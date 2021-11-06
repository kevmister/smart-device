/*
   SMART_DEVICE_BASE copyright of Kevin Gilmore
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "services.h"
#include "programs.h"

#define LED_PIN 2

#define DEVICE_CONFIGURATION_FILE "/spiffs/config.json"

static const char *TAG = "SMART_DEVICE_BASE";

cJSON *device_configuration;

static void configuration_read(){
	ESP_LOGI(TAG, "Reading configuration");
	FILE *file;
	file = fopen(DEVICE_CONFIGURATION_FILE, "r");

	if(file == NULL){
		return;
	}
	int buffer_size = 2048;
	char buffer[buffer_size];
	fread(buffer, sizeof(char), buffer_size, file);
	fclose(file);

	device_configuration = cJSON_Parse(buffer);
}

static void configuration_write(){
	ESP_LOGI(TAG, "Writing configuration");
	char *buffer;
	buffer = cJSON_Print(device_configuration);

	FILE* file;
	file = fopen(DEVICE_CONFIGURATION_FILE, "w");
	fputs(buffer, file);
	fclose(file);
}

//static esp_err_t smart_web_server_get_setup_scan(httpd_req_t *req){
//	gpio_set_level(LED_PIN, 1);
//	wifi_scan_config_t wifi_scan_config = {
//			.scan_type = WIFI_SCAN_TYPE_ACTIVE
//	};
//	uint16_t number_scanned;
//	ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, true));
//	esp_wifi_scan_get_ap_num(&number_scanned);
//
//	wifi_ap_record_t wifi_ap_records[number_scanned];
//
//	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number_scanned, wifi_ap_records));
//
//    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
//
//	const char header[] = "{\"stations\":[";
//	const char footer[] = "]}";
//
//	httpd_resp_send_chunk(req,header, strlen(header));
//
//	for(int i=0; i < number_scanned; i++){
//		char chunk[128];
//		sprintf(chunk, "{\"ssid\": \"%s\",\"rssi\": %d, \"encryption\": true }%s", wifi_ap_records[i].ssid, wifi_ap_records[i].rssi, i == number_scanned-1 ? "" : ",");
//		httpd_resp_send_chunk(req, chunk, strlen(chunk));
//	}
//	httpd_resp_send_chunk(req,footer, strlen(footer));
//	httpd_resp_send_chunk(req, NULL, 0);
//	gpio_set_level(LED_PIN, 0);
//
//    return ESP_OK;
//}

static void initialize_filesystem(){
	ESP_LOGI(TAG, "Initializing filesystem");
	size_t total,used;
	esp_vfs_spiffs_conf_t conf = {
	  .base_path = "/spiffs",
	  .partition_label = NULL,
	  .max_files = 5,
	  .format_if_mount_failed = false
	};
	ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
	ESP_ERROR_CHECK(esp_spiffs_info(NULL, &total, &used));
}

void app_main(){
	ESP_LOGI(TAG, "app_main begin");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    initialize_filesystem();
    configuration_read();

    cJSON *services_json, *programs_json;
    services_json = cJSON_GetObjectItem(device_configuration, "services");
    programs_json = cJSON_GetObjectItem(device_configuration, "programs");


	ESP_LOGI(TAG, "Processing services from configuration");

    if(services_json){
       cJSON *service_json = services_json->child;
       while(service_json){
    	   if(cJSON_GetObjectItem(service_json, "enabled")->valueint){
    		   service_t *service = service_get_by_name(service_json->string);
			   ESP_LOGI(TAG, "SERVICE_%s init", service_json->string);
			   if(service->init_handler() == SERVICE_INIT_FAILED){
				   ESP_LOGE(TAG, "SERVICE_%s init failed", service_json->string);
				   continue;
			   }
			   ESP_LOGI(TAG, "Service %s config", service_json->string);
			   if(service->config_handler(cJSON_GetObjectItem(service_json, "config")) == SERVICE_CONFIG_FAILED){
				   ESP_LOGE(TAG, "SERVICE_%s config failed", service_json->string);
				   continue;
			   }
    	   }
    	   service_json = service_json->next;
       }
    }

    ESP_LOGI(TAG, "Processing programs from configuration");

    service_wifi.start_handler();
    service_http_server.start_handler();
    service_mdns.start_handler();

    if(programs_json){
       cJSON *program_json = programs_json->child;
       while(program_json){
    	   if(cJSON_GetObjectItem(program_json, "enabled")->valueint){
    		   program_t *program = program_get_by_name(program_json->string);
			   ESP_LOGI(TAG, "PROGRAM_%s init", program_json->string);
			   if(program->init_handler() == PROGRAM_INIT_FAILED){
				   ESP_LOGE(TAG, "PROGRAM_%s init failed", program_json->string);
				   continue;
			   }
			   ESP_LOGI(TAG, "Service %s config", program_json->string);
			   if(program->config_handler(cJSON_GetObjectItem(program_json, "config")) == PROGRAM_CONFIG_FAILED){
				   ESP_LOGE(TAG, "PROGRAM_%s config failed", program_json->string);
				   continue;
			   }
    	   }
    	   program_json = program_json->next;
       }
    }


    program_air_suspension.start_handler();

    //service_mpu6050_interface_t *mpu6050 = service_mpu6050.interface;
    //int temperature = mpu6050->read_temperature();
    //ESP_LOGE(TAG, "TEMPERATURE LOGGED %d", temperature);
}
