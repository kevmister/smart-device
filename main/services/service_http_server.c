/*
 * service_http_server.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */

#include "service_http_server.h"
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_log.h"
#include "cJSON.h"

#define TAG "SERVICE_HTTP_SERVER"

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

httpd_handle_t server;
httpd_config_t httpd_config;

esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath){
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

esp_err_t handler_get_common(httpd_req_t *req){
	char filepath[256] = "/spiffs/www";
	char last_char = req->uri[strlen(req->uri) - 1];

	if(last_char == '/'){
		strlcat(filepath, "/index.html", sizeof(filepath));
	}else{
		strlcat(filepath, req->uri, sizeof(filepath));
	}

	int fd = open(filepath, O_RDONLY, 0);

	if(fd == -1){
		//ESP_LOGE(TAG, "Failed to open file: %s", filepath);
		httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "FILE NOT FOUND 404");
		return ESP_FAIL;
	}

	set_content_type_from_file(req, filepath);

	char buffer[512];
	ssize_t read_bytes;

	do {
		read_bytes = read(fd, buffer, sizeof(buffer));
		if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK){
			close(fd);
			//ESP_LOGE(TAG, "Failed to send file: %s", filepath);
			httpd_resp_sendstr_chunk(req, NULL);
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "INTERNAL SERVER ERROR 500");
			return ESP_FAIL;
		}

	} while(read_bytes == sizeof(buffer));

	close(fd);
	httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void post_connectivity_configuration(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
//	cJSON *access_point, *station;
//
//	access_point = cJSON_CreateObject();
//	cJSON_AddBoolToObject(access_point, "enabled", device_configuration.wifi.ap_enabled);
//	cJSON_AddStringToObject(access_point, "ssid", device_configuration.wifi.ap_ssid);
//	cJSON_AddStringToObject(access_point, "password", device_configuration.wifi.ap_password);
//	cJSON_AddItemToObject(res_root, "access_point", access_point);
//
//	station = cJSON_CreateObject();
//	cJSON_AddBoolToObject(station, "enabled", device_configuration.wifi.station_enabled);
//	cJSON_AddStringToObject(station, "ssid", device_configuration.wifi.station_ssid);
//	cJSON_AddStringToObject(station, "password", "********");
//	cJSON_AddItemToObject(res_root, "station", station);
}

void post_connectivity_status(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
//	cJSON *station;
//	wifi_ap_record_t wifi_ap_record;
//
//	ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_ap_record));
//
//	station = cJSON_CreateObject();
//	cJSON_AddBoolToObject(station, "connected", wifi_station_status == STATUS_CONNECTED ? true : false);
//	cJSON_AddStringToObject(station, "ip", wifi_station_ip);
//	cJSON_AddStringToObject(station, "ssid", device_configuration.wifi.station_ssid);
//	cJSON_AddItemToObject(res_root, "station", station);
}

void post_connectivity_apply(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
//	strncpy((char *)device_configuration.wifi.station_ssid, cJSON_GetObjectItem(req_root, "ssid")->valuestring, sizeof(device_configuration.wifi.station_ssid));
//	strncpy((char *)device_configuration.wifi.station_password, cJSON_GetObjectItem(req_root, "password")->valuestring, sizeof(device_configuration.wifi.station_password));
//	configuration_write();
//	cJSON_AddStringToObject(res_root, "status", "success");
}

void post_service_wifi_scan(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	wifi_scan_config_t wifi_scan_config = {
			.scan_type = WIFI_SCAN_TYPE_ACTIVE
	};
	uint16_t number_scanned;
	ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, true));
	esp_wifi_scan_get_ap_num(&number_scanned);

	wifi_ap_record_t wifi_ap_records[number_scanned];

	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number_scanned, wifi_ap_records));

	cJSON *stations;
	cJSON_AddBoolToObject(res_root, "success", true);

	stations = cJSON_CreateArray();
	cJSON_AddItemToObject(res_root, "stations", stations);

	for(int i=0; i < number_scanned; i++){
		cJSON* station;
		station = cJSON_CreateObject();
		cJSON_AddStringToObject(station, "ssid", (char *)wifi_ap_records[i].ssid);
		cJSON_AddNumberToObject(station, "rssi", (int)wifi_ap_records[i].rssi);
		cJSON_AddBoolToObject(station, "encryption", wifi_ap_records[i].group_cipher != WIFI_CIPHER_TYPE_NONE);
		cJSON_AddItemToArray(stations, station);
	}
}

void post_service_get_configuration(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	if(cJSON_GetObjectItem(req_root, "service")->valuestring == NULL){
		cJSON_AddBoolToObject(res_root, "success", false);
		cJSON_AddStringToObject(res_root, "error", "Service not specified");
		return;
	}
	service_t *service = service_get_by_name(cJSON_GetObjectItem(req_root, "service")->valuestring);
	if(!service){
		cJSON_AddBoolToObject(res_root, "success", false);
		cJSON_AddStringToObject(res_root, "error", "Service not found");
		return;
	}

	cJSON *config_reference;
	config_reference = cJSON_CreateObjectReference(service->service_config->child);
	cJSON_AddBoolToObject(res_root, "success", true);
	cJSON_AddItemToObject(res_root, "config", config_reference );
}

void post_service_set_configuration(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	if(cJSON_GetObjectItem(req_root, "service")->valuestring == NULL){
		cJSON_AddBoolToObject(res_root, "success", false);
		cJSON_AddStringToObject(res_root, "error", "Service not specified");
		return;
	}
	service_t *service = service_get_by_name(cJSON_GetObjectItem(req_root, "service")->valuestring);
	if(!service){
		cJSON_AddBoolToObject(res_root, "success", false);
		cJSON_AddStringToObject(res_root, "error", "Service not found");
		return;
	}

	cJSON_AddBoolToObject(res_root, "success", true);
	cJSON *config = cJSON_AddObjectToObject(res_root, "config");
	config->child = cJSON_CreateObjectReference(service->service_config);
}

esp_err_t handler_post_common(httpd_req_t *req){
	int content_length = req->content_len;
	char buffer[content_length];
	httpd_req_recv(req, buffer, content_length);
	buffer[content_length] = '\0';

	cJSON *req_root, *res_root;
	req_root = cJSON_Parse(buffer);
	res_root = cJSON_CreateObject();

	if(strcmp(req->uri, "/service/get_configuration") == 0){
		post_service_get_configuration(req, req_root, res_root);
	}
	if(strcmp(req->uri, "/service/set_configuration") == 0){
		post_service_set_configuration(req, req_root, res_root);
	}
	if(strcmp(req->uri, "/service/wifi/scan") == 0){
		post_service_wifi_scan(req, req_root, res_root);
	}
//	if(strcmp(req->uri, "/connectivity/apply") == 0){
//		post_connectivity_apply(req, req_root, res_root);
//	}
//	if(strcmp(req->uri, "/connectivity/scan") == 0){
//		post_connectivity_scan(req, req_root, res_root);
//	}

	httpd_resp_set_type(req, "application/json");
	httpd_resp_sendstr(req, cJSON_Print(res_root));
	cJSON_Delete(req_root);
	cJSON_Delete(res_root);
	return ESP_OK;
}

service_init_error_t service_http_server_init(){
	service_http_server.initialized = true;
	return SERVICE_INIT_OK;
}

service_config_error_t service_http_server_config(cJSON *service_config){
	if(!service_http_server.initialized){
		return SERVICE_CONFIG_FAILED;
	}
	httpd_config = (httpd_config_t)HTTPD_DEFAULT_CONFIG();
	httpd_config.uri_match_fn = httpd_uri_match_wildcard;
	httpd_config.stack_size = 20480;
	httpd_config.server_port = (uint16_t)cJSON_GetObjectItem(service_config, "port")->valueint;
	service_http_server.configured = true;
	service_http_server.service_config = service_config;
	return SERVICE_CONFIG_OK;
}

service_start_error_t service_http_server_start(){
	if(!service_http_server.configured){
		ESP_LOGE(TAG, "Cannot start, not configured");
		return SERVICE_START_FAILED;
	}
	if(service_http_server.started){
		ESP_LOGE(TAG, "Cannot start, already started");
		return SERVICE_START_FAILED;
		}
	if(!service_wifi.started){
		ESP_LOGE(TAG, "Cannot start, SERVICE_WIFI not started");
		return SERVICE_START_FAILED;
	}
	if(httpd_start(&server, &httpd_config) != ESP_OK){
		ESP_LOGE(TAG, "Cannot start, error starting httpd");
		return SERVICE_START_FAILED;
	}
	httpd_uri_t common_post_uri = {
			.uri = "/*",
			.method = HTTP_POST,
			.handler = handler_post_common,
			.user_ctx = NULL
	};
	httpd_uri_t common_get_uri = {
			.uri = "/*",
			.method = HTTP_GET,
			.handler = handler_get_common,
			.user_ctx = NULL
	};
	httpd_register_uri_handler(server, &common_post_uri);
	httpd_register_uri_handler(server, &common_get_uri);
	ESP_LOGI(TAG, "Server started");
	service_http_server.started = true;
	return SERVICE_START_OK;
}

service_stop_error_t service_http_server_stop(){
	httpd_stop(server);
	service_http_server.started = false;
	return SERVICE_STOP_OK;
}

service_end_error_t service_http_server_end(){
	if(!service_http_server.initialized){
		return SERVICE_END_FAILED;
	}
	service_http_server.initialized = false;
	service_http_server.configured = false;
	service_http_server.started = false;
	return SERVICE_END_OK;
}

service_t service_http_server = {
		.name = "HTTP_SERVER",
		.initialized = false,
		.configured = false,
		.started = false,
		.init_handler = service_http_server_init,
		.config_handler = service_http_server_config,
		.start_handler = service_http_server_start,
		.stop_handler = service_http_server_stop,
		.end_handler = service_http_server_end
};

