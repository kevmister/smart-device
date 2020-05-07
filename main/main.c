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
#include "esp_system.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "tcpip_adapter.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define LED_PIN 2

typedef struct {
	struct {
		bool ap_enabled;
		bool station_enabled;
		char station_ssid[32];
		char station_password[64];
		char ap_ssid[32];
		char ap_password[64];
	} wifi;
} device_configuration_t;

typedef enum {
	STATUS_CONNECTING,
	STATUS_CONNECTED
} wifi_station_status_t;

wifi_station_status_t wifi_station_status;

static EventGroupHandle_t wifi_setup_event_group;
int wifi_setup_connection_attemps = 0;

#define SMART_WIFI_STATION_MAX_RECONNECTION_ATTEMPTS 4
#define WIFI_SETUP_PASSED_BIT BIT0
#define WIFI_SETUP_FAILED_CONNECT_BIT BIT1

#define DEVICE_CONFIGURATION_FILE "/spiffs/config.json"

wifi_config_t wifi_config_ap;
wifi_config_t wifi_config_sta;
tcpip_adapter_ip_info_t tcpip_adapter_ip_info;
char *wifi_station_ip;

device_configuration_t device_configuration = {
	.wifi = {
			.ap_enabled = true,
			.station_enabled = true,
			.station_ssid = "Gilmore Basement",
			.station_password = "GilmoreHaun!?",
			.ap_ssid = "SMART_GILMORE",
			.ap_password = "password"
	}
};

static const char *TAG = "SMART_DEVICE_BASE";

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static void configuration_read(){
	FILE *file;
	file = fopen(DEVICE_CONFIGURATION_FILE, "r");

	if(file == NULL){
		return;
	}

	char buffer[512];
	fread(buffer, sizeof(char), 512, file);
	fclose(file);

	cJSON *root, *wifi;
	root = cJSON_Parse(buffer);
	wifi = cJSON_GetObjectItemCaseSensitive(root, "wifi");

	device_configuration.wifi.station_enabled = cJSON_IsTrue(cJSON_GetObjectItem(wifi, "station_enabled")) != 0;
	strncpy((char *) device_configuration.wifi.station_ssid, cJSON_GetObjectItem(wifi, "station_ssid")->valuestring, sizeof(device_configuration.wifi.station_ssid));
	strncpy((char *) device_configuration.wifi.station_password, cJSON_GetObjectItem(wifi, "station_password")->valuestring, sizeof(device_configuration.wifi.station_password));
	device_configuration.wifi.ap_enabled = cJSON_IsTrue(cJSON_GetObjectItem(wifi, "ap_enabled")) != 0;
	strncpy((char *) device_configuration.wifi.ap_ssid, cJSON_GetObjectItem(wifi, "ap_ssid")->valuestring, sizeof(device_configuration.wifi.ap_ssid));
	strncpy((char *) device_configuration.wifi.ap_password, cJSON_GetObjectItem(wifi, "ap_password")->valuestring, sizeof(device_configuration.wifi.ap_password));

	cJSON_Delete(root);
}

static void configuration_write(){
	char *buffer;
	cJSON *root,*wifi;
	root = cJSON_CreateObject();
	wifi = cJSON_CreateObject();
	cJSON_AddItemToObject(wifi, "wifi", root);
	cJSON_AddItemToObject(wifi, "station_ssid", cJSON_CreateString(device_configuration.wifi.station_ssid));
	cJSON_AddItemToObject(wifi, "station_password", cJSON_CreateString(device_configuration.wifi.station_password));
	buffer = cJSON_Print(root);
	cJSON_Delete(root);

	FILE* file;
	file = fopen(DEVICE_CONFIGURATION_FILE, "w");
	fputs(buffer, file);
	fclose(file);
}

static void initialize_sntp(){
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
	sntp_init();
}

static void smart_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	if(event_base == WIFI_EVENT){
		if(event_id == WIFI_EVENT_STA_START){
			if(device_configuration.wifi.station_enabled){
				wifi_station_status = STATUS_CONNECTING;
				ESP_LOGI(TAG, "WiFi station connecting");
				esp_wifi_connect();
			}
		}
		if(event_id == WIFI_EVENT_STA_DISCONNECTED){
			if(!device_configuration.wifi.station_enabled){
				if(++wifi_setup_connection_attemps == SMART_WIFI_STATION_MAX_RECONNECTION_ATTEMPTS){
					wifi_setup_connection_attemps = 0;
					xEventGroupSetBits(wifi_setup_event_group, WIFI_SETUP_FAILED_CONNECT_BIT);
				}else{
					esp_wifi_connect();
				}
			}else{
				wifi_station_status = STATUS_CONNECTING;
				ESP_LOGI(TAG, "WiFi station reconnecting");
				esp_wifi_connect();
			}
		}
		if(event_id == WIFI_EVENT_AP_STACONNECTED){
			ESP_LOGI(TAG, "WiFi AP connected");
		}
		if(event_id == WIFI_EVENT_AP_STADISCONNECTED){
			ESP_LOGI(TAG, "WiFi AP disconnected");
		}
	}
	if(event_base == IP_EVENT){
		if(event_id == IP_EVENT_STA_GOT_IP){
			ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
			wifi_station_ip = (char *)ip4addr_ntoa(&event->ip_info.ip);
			if(!device_configuration.wifi.station_enabled){
				xEventGroupSetBits(wifi_setup_event_group, WIFI_SETUP_PASSED_BIT);
				wifi_setup_connection_attemps = 0;
			}
			wifi_station_status = STATUS_CONNECTED;
			ESP_LOGI(TAG, "WiFi station connected");
		}
	}
}

static void configure_wifi(){
	uint8_t mac_address[6];

	wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	wifi_config_ap.ap.max_connection = 4;
	strncpy((char *)wifi_config_ap.ap.ssid, device_configuration.wifi.ap_ssid, sizeof(wifi_config_ap.ap.ssid));
	strncpy((char *)wifi_config_ap.ap.password, device_configuration.wifi.ap_password, sizeof(wifi_config_ap.ap.password));
	strncpy((char *)wifi_config_sta.sta.ssid, device_configuration.wifi.station_ssid, sizeof(wifi_config_sta.sta.ssid));
	strncpy((char *)wifi_config_sta.sta.password, device_configuration.wifi.station_password, sizeof(wifi_config_sta.sta.password));

	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP, mac_address));

	wifi_mode_t wifi_mode;
	ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));

	if(device_configuration.wifi.ap_enabled && device_configuration.wifi.station_enabled){
		ESP_LOGI(TAG, "Starting WiFi in WIFI_MODE_APSTA, AP(%s : %s) STA(%s : %s)", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password, wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
		ESP_ERROR_CHECK(esp_wifi_start());
	}
	else if(device_configuration.wifi.ap_enabled && !device_configuration.wifi.station_enabled){
		ESP_LOGI(TAG, "Starting WiFi in WIFI_MODE_AP, AP(%s : %s)", wifi_config_ap.ap.ssid, wifi_config_ap.ap.password);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &tcpip_adapter_ip_info));
		ESP_ERROR_CHECK(esp_wifi_start());
	}
	else if(!device_configuration.wifi.ap_enabled && device_configuration.wifi.station_enabled){
		ESP_LOGI(TAG, "Starting WiFi in WIFI_MODE_STA, STA(%s : %s)", wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta));
		ESP_ERROR_CHECK(tcpip_adapter_ap_start(mac_address, &tcpip_adapter_ip_info));
		ESP_ERROR_CHECK(esp_wifi_start());
	} else {
		ESP_ERROR_CHECK(esp_wifi_stop());
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
	}
}

static void initialize_network(){
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &smart_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &smart_wifi_event_handler, NULL));
	IP4_ADDR(&tcpip_adapter_ip_info.ip, 192,168,201,1);
	IP4_ADDR(&tcpip_adapter_ip_info.netmask, 255,255,255,0);
	IP4_ADDR(&tcpip_adapter_ip_info.gw, 192,168,201,1);
	configure_wifi();
	initialize_sntp();
}

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

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
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

static esp_err_t handler_get_common(httpd_req_t *req){
	char filepath[256] = "/spiffs/www";
	char last_char = req->uri[strlen(req->uri) - 1];

	if(last_char == '/'){
		strlcat(filepath, "/index.html", sizeof(filepath));
	}else{
		strlcat(filepath, req->uri, sizeof(filepath));
	}

	int fd = open(filepath, O_RDONLY, 0);

	if(fd == -1){
		ESP_LOGE(TAG, "Failed to open file: %s", filepath);
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
			ESP_LOGE(TAG, "Failed to send file: %s", filepath);
			httpd_resp_sendstr_chunk(req, NULL);
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "INTERNAL SERVER ERROR 500");
			return ESP_FAIL;
		}

	} while(read_bytes == sizeof(buffer));

	close(fd);
	httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static void post_connectivity_configuration(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	cJSON *access_point, *station;

	access_point = cJSON_CreateObject();
	cJSON_AddBoolToObject(access_point, "enabled", device_configuration.wifi.ap_enabled);
	cJSON_AddStringToObject(access_point, "ssid", device_configuration.wifi.ap_ssid);
	cJSON_AddStringToObject(access_point, "password", device_configuration.wifi.ap_password);
	cJSON_AddItemToObject(res_root, "access_point", access_point);

	station = cJSON_CreateObject();
	cJSON_AddBoolToObject(station, "enabled", device_configuration.wifi.station_enabled);
	cJSON_AddStringToObject(station, "ssid", device_configuration.wifi.station_ssid);
	cJSON_AddStringToObject(station, "password", "********");
	cJSON_AddItemToObject(res_root, "station", station);
}

static void post_connectivity_status(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	cJSON *station;
	wifi_ap_record_t wifi_ap_record;

	ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_ap_record));

	station = cJSON_CreateObject();
	cJSON_AddBoolToObject(station, "connected", wifi_station_status == STATUS_CONNECTED ? true : false);
	cJSON_AddStringToObject(station, "ip", wifi_station_ip);
	cJSON_AddStringToObject(station, "ssid", device_configuration.wifi.station_ssid);
	cJSON_AddItemToObject(res_root, "station", station);
}

static void post_connectivity_apply(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	strncpy((char *)device_configuration.wifi.station_ssid, cJSON_GetObjectItem(req_root, "ssid")->valuestring, sizeof(device_configuration.wifi.station_ssid));
	strncpy((char *)device_configuration.wifi.station_password, cJSON_GetObjectItem(req_root, "password")->valuestring, sizeof(device_configuration.wifi.station_password));
	configuration_write();
	cJSON_AddStringToObject(res_root, "status", "success");
}

static void post_connectivity_scan(httpd_req_t *req, cJSON *req_root, cJSON *res_root){
	wifi_scan_config_t wifi_scan_config = {
			.scan_type = WIFI_SCAN_TYPE_ACTIVE
	};
	uint16_t number_scanned;
	ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, true));
	esp_wifi_scan_get_ap_num(&number_scanned);

	wifi_ap_record_t wifi_ap_records[number_scanned];

	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number_scanned, wifi_ap_records));

	cJSON *stations;

	stations = cJSON_CreateArray();
	cJSON_AddItemToObject(res_root, "station", stations);

	for(int i=0; i < number_scanned; i++){
		cJSON* station;
		station = cJSON_CreateObject();
		cJSON_AddStringToObject(station, "ssid", (char *)wifi_ap_records[i].ssid);
		cJSON_AddNumberToObject(station, "rssi", (int)wifi_ap_records[i].rssi);
		cJSON_AddBoolToObject(station, "encryption", wifi_ap_records[i].group_cipher != WIFI_CIPHER_TYPE_NONE);
		cJSON_AddItemToArray(stations, station);
	}
}

static esp_err_t handler_post_common(httpd_req_t *req){
	int content_length = req->content_len;
	char buffer[content_length];
	httpd_req_recv(req, buffer, content_length);
	buffer[content_length] = '\0';

	cJSON *req_root, *res_root;
	req_root = cJSON_Parse(buffer);
	res_root = cJSON_CreateObject();

	if(strcmp(req->uri, "/connectivity/get_configuration") == 0){
		post_connectivity_configuration(req, req_root, res_root);
	}
	if(strcmp(req->uri, "/connectivity/get_status") == 0){
		post_connectivity_status(req, req_root, res_root);
	}
	if(strcmp(req->uri, "/connectivity/apply") == 0){
		post_connectivity_apply(req, req_root, res_root);
	}
	if(strcmp(req->uri, "/connectivity/scan") == 0){
		post_connectivity_scan(req, req_root, res_root);
	}

	httpd_resp_set_type(req, "application/json");
	httpd_resp_sendstr(req, cJSON_Print(res_root));
	cJSON_Delete(req_root);
	cJSON_Delete(res_root);
	return ESP_OK;
}

static httpd_handle_t initialize_web_server(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 20480;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
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
    }
    return server;
}

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
	gpio_pad_select_gpio(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    configuration_read();
    initialize_network();
    initialize_filesystem();
    initialize_web_server();
}
