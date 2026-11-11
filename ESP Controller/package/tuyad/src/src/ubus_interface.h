
#ifndef __UBUS_INTERFACE_H
#define __UBUS_INTERFACE_H

int init_ubus_client(void);
void deinit_ubus_client(void);

#define UBUS_MAX_ESP_DEVICES 32
#define UBUS_DEVICE_STRING_LENGTH 64
#define UBUS_ESP_MESSAGE_LENGTH 128

struct ubus_device_result {
	char port[UBUS_DEVICE_STRING_LENGTH];
	char vendor_id[UBUS_DEVICE_STRING_LENGTH];
	char product_id[UBUS_DEVICE_STRING_LENGTH];
};

struct ubus_toggle_result {
	int response_code;
	char message[UBUS_ESP_MESSAGE_LENGTH];
};

struct ubus_get_result {
	int response_code;
	char message[UBUS_ESP_MESSAGE_LENGTH];

	int has_data;
	int humidity;
	double temperature;
};

typedef void (*ubus_devices_cb)(struct ubus_device_result *devices, int num_devices);
typedef void (*ubus_toggle_pin_cb)(int enable, struct ubus_toggle_result *result);
typedef void (*ubus_get_cb)(struct ubus_get_result *result);

int ubus_devices(ubus_devices_cb callback);
int ubus_enable_pin(const char *port, int pin, ubus_toggle_pin_cb callback);
int ubus_disable_pin(const char *port, int pin, ubus_toggle_pin_cb callback);
int ubus_get(const char *port, int pin, const char *model, const char *sensor, ubus_get_cb callback);

#endif