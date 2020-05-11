/*
 * service_wifi.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_wifi.h"

#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "cJSON.h"

#define TAG "SERVICE_WIFI"

char *station_ip;

void service_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	if(event_base == WIFI_EVENT){
		if(event_id == WIFI_EVENT_STA_START){
			ESP_LOGI(TAG, "Station started, connecting");
			esp_wifi_connect();
		}
		if(event_id == WIFI_EVENT_STA_DISCONNECTED){
			ESP_LOGI(TAG, "Station disconnected, reconnecting ");
			esp_wifi_connect();
		}
		if(event_id == WIFI_EVENT_STA_CONNECTED){
			ESP_LOGI(TAG, "Station connected");
		}
		if(event_id == WIFI_EVENT_AP_STACONNECTED){}
		if(event_id == WIFI_EVENT_AP_STADISCONNECTED){}
	}
	if(event_base == IP_EVENT){
		if(event_id == IP_EVENT_STA_GOT_IP){
			ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
			station_ip = (char *)ip4addr_ntoa(&event->ip_info.ip);
			ESP_LOGI(TAG, "Station got address %s", station_ip);
		}
	}
}

service_init_error_t service_wifi_init(){
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &service_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &service_wifi_event_handler, NULL));
	service_wifi.initialized = true;
	return SERVICE_INIT_OK;
}
service_config_error_t service_wifi_config(cJSON *service_config){
	if(!service_wifi.initialized){
		return SERVICE_CONFIG_FAILED;
	}

	uint8_t mac_address[6];
	wifi_config_t wifi_config_ap = {
			.ap = {
				.ssid = "",
				.password = ""
			}
	};
	wifi_config_t wifi_config_sta = {
			.sta = {
				.ssid = "",
				.password = ""
			}
	};
	tcpip_adapter_ip_info_t tcpip_adapter_ip_info;

	IP4_ADDR(&tcpip_adapter_ip_info.ip, 192,168,201,1);
	IP4_ADDR(&tcpip_adapter_ip_info.netmask, 255,255,255,0);
	IP4_ADDR(&tcpip_adapter_ip_info.gw, 192,168,201,1);
	wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	wifi_config_ap.ap.max_connection = 4;

	if(cJSON_GetObjectItem(service_config, "ap_enabled")->valueint){
		if(cJSON_GetObjectItem(service_config, "ap_ssid")->valuestring == NULL || cJSON_GetObjectItem(service_config, "ap_password")->valuestring == NULL){
			return SERVICE_CONFIG_FAILED;
		}
	}

	if(cJSON_GetObjectItem(service_config, "sta_enabled")->valueint){
		if(cJSON_GetObjectItem(service_config, "sta_ssid")->valuestring == NULL || cJSON_GetObjectItem(service_config, "sta_password")->valuestring == NULL){
			return SERVICE_CONFIG_FAILED;
		}
	}

	strncpy((char *)wifi_config_ap.ap.ssid, cJSON_GetObjectItem(service_config, "ap_ssid")->valuestring, sizeof(wifi_config_ap.ap.ssid));
	strncpy((char *)wifi_config_ap.ap.password, cJSON_GetObjectItem(service_config, "ap_password")->valuestring, sizeof(wifi_config_ap.ap.password));
	strncpy((char *)wifi_config_sta.sta.ssid, cJSON_GetObjectItem(service_config, "sta_ssid")->valuestring, sizeof(wifi_config_sta.sta.ssid));
	strncpy((char *)wifi_config_sta.sta.password, cJSON_GetObjectItem(service_config, "sta_password")->valuestring, sizeof(wifi_config_sta.sta.password));

	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP, mac_address));

	if(cJSON_GetObjectItem(service_config, "ap_enabled")->valueint && cJSON_GetObjectItem(service_config, "sta_enabled")->valueint){
		ESP_LOGI(TAG, "Configuring WiFi in WIFI_MODE_APSTA, AP(%s : %s) STA(%s : %s)", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password, wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
	}
	else if(cJSON_GetObjectItem(service_config, "ap_enabled")->valueint && !cJSON_GetObjectItem(service_config, "sta_enabled")->valueint){
		ESP_LOGI(TAG, "Configuring WiFi in WIFI_MODE_AP, AP(%s : %s)", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &tcpip_adapter_ip_info));
	}
	else if(!cJSON_GetObjectItem(service_config, "ap_enabled")->valueint && cJSON_GetObjectItem(service_config, "sta_enabled")->valueint){
		ESP_LOGI(TAG, "Configuring WiFi in WIFI_MODE_STA, STA(%s : %s)", wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &tcpip_adapter_ip_info));
	} else {
		ESP_LOGE(TAG, "Cannot start, AP or STA mode must be enabled");
		return SERVICE_CONFIG_FAILED;
	}
	service_wifi.configured = true;
	service_wifi.service_config = service_config;
	return SERVICE_CONFIG_OK;
}
service_start_error_t service_wifi_start(){
	if(!service_wifi.configured || service_wifi.started){
		return SERVICE_START_FAILED;
	}
	ESP_ERROR_CHECK(esp_wifi_start());
	service_wifi.started = true;
	return SERVICE_START_OK;
}
service_stop_error_t service_wifi_stop(){
	if(!service_wifi.started){
		return SERVICE_STOP_FAILED;
	}
	ESP_ERROR_CHECK(esp_wifi_stop());
	return SERVICE_STOP_OK;
}
service_end_error_t service_wifi_end(){
	return SERVICE_END_OK;
}

service_t service_wifi = {
		.name = "WIFI",
		.initialized = 0,
		.configured = 0,
		.started = 0,
		.init_handler = service_wifi_init,
		.config_handler = service_wifi_config,
		.start_handler = service_wifi_start,
		.stop_handler = service_wifi_stop,
		.end_handler = service_wifi_end
};
