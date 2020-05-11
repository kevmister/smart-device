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

#define LED_PIN 2

#define DEVICE_CONFIGURATION_FILE "/spiffs/config.json"

static const char *TAG = "SMART_DEVICE_BASE";

cJSON *device_configuration;

static void configuration_read(){
	FILE *file;
	file = fopen(DEVICE_CONFIGURATION_FILE, "r");

	if(file == NULL){
		return;
	}

	char buffer[1024];
	fread(buffer, sizeof(char), 1024, file);
	fclose(file);

	device_configuration = cJSON_Parse(buffer);
}

static void configuration_write(){
	char *buffer;
	buffer = cJSON_Print(device_configuration);

	FILE* file;
	file = fopen(DEVICE_CONFIGURATION_FILE, "w");
	fputs(buffer, file);
	fclose(file);
}

//static void configure_wifi(){
//	uint8_t mac_address[6];
//
//	wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
//	wifi_config_ap.ap.max_connection = 4;
//	strncpy((char *)wifi_config_ap.ap.ssid, device_configuration.wifi.ap_ssid, sizeof(wifi_config_ap.ap.ssid));
//	strncpy((char *)wifi_config_ap.ap.password, device_configuration.wifi.ap_password, sizeof(wifi_config_ap.ap.password));
//	strncpy((char *)wifi_config_sta.sta.ssid, device_configuration.wifi.station_ssid, sizeof(wifi_config_sta.sta.ssid));
//	strncpy((char *)wifi_config_sta.sta.password, device_configuration.wifi.station_password, sizeof(wifi_config_sta.sta.password));
//
//	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP, mac_address));
//
//	wifi_mode_t wifi_mode;
//	ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));
//
//	if(device_configuration.wifi.ap_enabled && device_configuration.wifi.station_enabled){
//		ESP_LOGI(TAG, "Starting WiFi in WIFI_MODE_APSTA, AP(%s : %s) STA(%s : %s)", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password, wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
//		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
//		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
//		ESP_ERROR_CHECK(esp_wifi_start());
//	}
//	else if(device_configuration.wifi.ap_enabled && !device_configuration.wifi.station_enabled){
//		ESP_LOGI(TAG, "Starting WiFi in WIFI_MODE_AP, AP(%s : %s)", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password);
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
//		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
//		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &tcpip_adapter_ip_info));
//		ESP_ERROR_CHECK(esp_wifi_start());
//	}
//	else if(!device_configuration.wifi.ap_enabled && device_configuration.wifi.station_enabled){
//		ESP_LOGI(TAG, "Starting WiFi in WIFI_MODE_STA, STA(%s : %s)", wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
//		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &tcpip_adapter_ip_info));
//		ESP_ERROR_CHECK(esp_wifi_start());
//	} else {
//		ESP_ERROR_CHECK(esp_wifi_stop());
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
//	}
//}

//static void initialize_network(){
//	tcpip_adapter_init();
//	ESP_ERROR_CHECK(esp_event_loop_create_default());
//	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smart_wifi_event_handler, NULL));
//	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smart_wifi_event_handler, NULL));
//	IP4_ADDR(&tcpip_adapter_ip_info.ip, 192,168,201,1);
//	IP4_ADDR(&tcpip_adapter_ip_info.netmask, 255,255,255,0);
//	IP4_ADDR(&tcpip_adapter_ip_info.gw, 192,168,201,1);
//	configure_wifi();
//	initialize_sntp();
//}

//static void smart_wifi_set_mode(){
//	if(mode == smart_wifi_mode){
//		ESP_LOGI(TAG, "No change in wifi mode");
//		return;
//	}
//
//	if(mode == SMART_WIFI_MODE_OFF){
//		ESP_LOGI(TAG, "Turning wifi off");
//		smart_wifi_ap_state = SMART_WIFI_AP_STATE_DISABLED;
//		smart_wifi_station_state = SMART_WIFI_STATION_STATE_DISABLED;
//		ESP_ERROR_CHECK(esp_wifi_disconnect());
//		ESP_ERROR_CHECK(esp_wifi_stop());
//	}
//
//	if(mode == SMART_WIFI_MODE_AP){
//		ESP_LOGI(TAG, "Starting wifi in AP mode");
//		int ssid_length = strlen(SMART_WIFI_AP_SSID_PREFIX)+6;
//		char ssid[ssid_length+1];
//		uint8_t mac_address[6];
//		ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP, mac_address));
//		sprintf(ssid, "%s_%2X:%2X", SMART_WIFI_AP_SSID_PREFIX, mac_address[4], mac_address[5]);
//
//		wifi_config_t wifi_config = {
//			.ap = {
//				.ssid = "",
//				.ssid_len = ssid_length,
//				.password = SMART_WIFI_AP_DEFAULT_PASSWORD,
//				.max_connection = SMART_WIFI_AP_MAX_CONNECTIONS,
//				.authmode = WIFI_AUTH_WPA_WPA2_PSK
//			},
//		};
//
//		strncpy((char *) wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
//
//		if(strlen(SMART_WIFI_AP_DEFAULT_PASSWORD) == 0) {
//			wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//		}
//
//
//		if(smart_wifi_mode != SMART_WIFI_MODE_OFF)
//			ESP_ERROR_CHECK(esp_wifi_stop());
//
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
//		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
//
//		tcpip_adapter_ip_info_t ip_info;
//		IP4_ADDR(&ip_info.ip, 192,168,201,1);
//		IP4_ADDR(&ip_info.netmask, 255,255,255,0);
//		IP4_ADDR(&ip_info.gw, 192,168,201,1);
//
//		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &ip_info));
//		ESP_ERROR_CHECK(esp_wifi_start());
//		ESP_ERROR_CHECK(esp_wifi_disconnect());
//	}
//
//	if(mode == SMART_WIFI_MODE_STATION){
//		ESP_LOGI(TAG, "Starting wifi in Station mode");
//		wifi_config_t wifi_config = {
//			.sta = {
//				.ssid = "",
//				.password = ""
//			},
//		};
//
//		strncpy((char *) wifi_config.sta.ssid, smart_device_config.wifi_station_ssid, sizeof(wifi_config.sta.ssid));
//		strncpy((char *) wifi_config.sta.password, smart_device_config.wifi_station_password, sizeof(wifi_config.sta.password));
//		if(smart_wifi_mode != SMART_WIFI_MODE_OFF){
//			ESP_ERROR_CHECK(esp_wifi_stop());
//		}
//
//
//		smart_wifi_ap_state = SMART_WIFI_AP_STATE_ENABLED;
//
//		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//		ESP_ERROR_CHECK(esp_wifi_start());
//	}
//
//	smart_wifi_mode = mode;
//}


//static esp_err_t smart_web_server_get_root(httpd_req_t *req){
//	FILE* file;
//	char buffer[2048];
//	size_t read;
//
//	file = fopen("/spiffs/application.html", "r");
//
//	do {
//		read = fread(buffer, 1, sizeof(buffer), file);
//		httpd_resp_send_chunk(req, buffer, read);
//	} while(read == sizeof(buffer));
//
//	fclose(file);
//
//	httpd_resp_send_chunk(req, NULL, 0);
//    return ESP_OK;
//}

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
//
//static esp_err_t smart_web_server_post_setup_check(httpd_req_t *req){
//	wifi_setup_event_group = xEventGroupCreate();
////	int content_length = req->content_len < 128 ? req->content_len : 128;
////	char content[content_length];
////	int ret = httpd_req_recv(req, content, content_length);
////	if(ret == 0){
////		return ESP_FAIL;
////	}
////
////	cJSON *root = cJSON_Parse(content);
////	char *ssid;
////	char *password;
////	ssid = cJSON_GetObjectItem(root, "ssid")->valuestring;
////	password = cJSON_GetObjectItem(root, "password")->valuestring;
////	cJSON_Delete(root);
//
//	char *ssid = "Gilmore Basement";
//	char *password = "GilmoreHaun!?";
//	wifi_config_t wifi_config = {
//			.sta = {
//				.ssid = "",
//				.password = ""
//			},
//	};
//
//	strncpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
//	strncpy((char *) wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
//
//	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//	ESP_ERROR_CHECK(esp_wifi_connect());
//	EventBits_t connect_bits = xEventGroupWaitBits(wifi_setup_event_group, WIFI_SETUP_PASSED_BIT | WIFI_SETUP_FAILED_CONNECT_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
//	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
//	char chunk[64];
//	sprintf(chunk, "{ \"status\": { \"passed\": %s }}", connect_bits & WIFI_SETUP_PASSED_BIT ? "true": "false");
//	httpd_resp_send(req, chunk, strlen(chunk));
//	vEventGroupDelete(wifi_setup_event_group);
//	return ESP_OK;
//}

static void initialize_filesystem(){
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
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    initialize_filesystem();
    configuration_read();

    cJSON *services_json;
    services_json = cJSON_GetObjectItem(device_configuration, "services");

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

    service_wifi.start_handler();
    service_http_server.start_handler();

}
