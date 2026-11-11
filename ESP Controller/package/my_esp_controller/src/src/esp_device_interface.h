
#ifndef __ESP_DEVICE_INTERFACE_H
#define __ESP_DEVICE_INTERFACE_H

#include <cJSON.h>

struct esp_device_info {
	char *port_name;
	int vendor_id;
	int product_id;
};

int get_esp_device_info(struct esp_device_info *device_info, int num_requested_device_infos);

// Returns a 'handle' to an esp device. Caller must free using free_esp_device
void *get_esp_device_by_port_name(const char *port_name);
void free_esp_device(void *dev_handle);

int esp_device_toggle_pin(void *dev_handle, int pin, int enable, cJSON **json_response);
int esp_device_get_data(void *dev_handle, int pin, const char *model, const char *sensor, cJSON **json_response);

#endif
