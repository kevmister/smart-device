/*
 * wifi.c
 *
 *  Created on: May 8, 2020
 *      Author: kevmi
 */


service_init_error_t service_wifi_init(){
	return SERVICE_INIT_OK;
}
service_config_error_t service_wifi_config(){
	return SERVICE_CONFIG_OK;
}
service_start_error_t service_wifi_start(){
	return SERVICE_START_OK;
}
service_stop_error_t service_wifi_stop(){
	return SERVICE_STOP_OK;
}

service_t service_mdns = {
		.state = SERVICE_STATE_NOT_INITIALIZED,
		.service_init = service_wifi_init(),
		.service_config = service_wifi_config(),
		.service_start = service_wifi_start(),
		.service_stop = service_wifi_stop()
};
