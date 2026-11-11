
#include <stdio.h>
#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include <syslog.h>

#include "ubus_interface.h"

#define UBUS_CALL_TIMEOUT 10000

static struct ubus_context *ubus_context = NULL;
static int service_object_id = -1;

static int ubus_lookup_object_id(const char *object_name)
{
	unsigned int ret = -1;
	if (ubus_lookup_id(ubus_context, object_name, &ret)) {
		syslog(LOG_ERR, "Failed to lookup %s ubus object\n", object_name);
		return 1;
	}

	return ret;
}

int init_ubus_client(void)
{
	ubus_context = ubus_connect(NULL);

	if (ubus_context == NULL) {
		syslog(LOG_ERR, "Failed connecting to ubus server\n");
		return 1;
	}

	service_object_id = ubus_lookup_object_id("my_esp_controller");
	if (service_object_id == -1) {
		syslog(LOG_ERR, "Failed finding ubus service");
		return 1;
	}

	return 0;
}

void deinit_ubus_client(void)
{
	if (ubus_context) {
		ubus_free(ubus_context);
		ubus_context = NULL;
	}

	service_object_id = -1;
}

enum { DEVICES_LIST, _DEVICES_MAX };
static int parse_devices_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy devices_policy[_DEVICES_MAX] = {
		[DEVICES_LIST] = { .name = "devices", .type = BLOBMSG_TYPE_ARRAY }
	};

	if (blobmsg_parse(devices_policy, _DEVICES_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) != 0) {
		syslog(LOG_ERR, "Parse devices info failed\n");
		return 1;
	}

	if (attr[DEVICES_LIST] == NULL) {
		syslog(LOG_ERR, "No devices list\n");
		return 1;
	}

	return 0;
}

enum { DEVICE_PORT, DEVICE_VENDOR_ID, DEVICE_PRODUCT_ID, _DEVICE_MAX };
static int parse_device_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy device_policy[_DEVICE_MAX] = {
		[DEVICE_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
		[DEVICE_VENDOR_ID] = { .name = "vendor_id", .type = BLOBMSG_TYPE_STRING },
		[DEVICE_PRODUCT_ID] = { .name = "product_id", .type = BLOBMSG_TYPE_STRING }
	};

	if (blobmsg_parse(device_policy, _DEVICE_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) != 0) {
		syslog(LOG_ERR, "Parse device info failed\n");
		return 1;
	}

	if (attr[DEVICE_PORT] == NULL) {
		syslog(LOG_ERR, "No device port\n");
		return 1;
	}

	if (attr[DEVICE_VENDOR_ID] == NULL) {
		syslog(LOG_ERR, "No device vendor id\n");
		return 1;
	}

	if (attr[DEVICE_PRODUCT_ID] == NULL) {
		syslog(LOG_ERR, "No device product id\n");
		return 1;
	}

	return 0;
}

static void devices_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct blob_attr *devices_tb[_DEVICES_MAX];
	if (parse_devices_info(devices_tb, msg))
		return;

	struct ubus_device_result devices[UBUS_MAX_ESP_DEVICES];
	int num_devices = 0;

	struct blob_attr *cur;
	int rem;
	blobmsg_for_each_attr(cur, devices_tb[DEVICES_LIST], rem)
	{
		if (num_devices >= UBUS_MAX_ESP_DEVICES)
			break;

		struct blob_attr *device_tb[_DEVICE_MAX];
		if (parse_device_info(device_tb, cur))
			continue;

		const char *port = blobmsg_get_string(device_tb[DEVICE_PORT]);
		strlcpy(devices[num_devices].port, port, UBUS_DEVICE_STRING_LENGTH);

		const char *vendor_id = blobmsg_get_string(device_tb[DEVICE_VENDOR_ID]);
		strlcpy(devices[num_devices].vendor_id, vendor_id, UBUS_DEVICE_STRING_LENGTH);

		const char *product_id = blobmsg_get_string(device_tb[DEVICE_PRODUCT_ID]);
		strlcpy(devices[num_devices].product_id, product_id, UBUS_DEVICE_STRING_LENGTH);

		num_devices++;
	}

	ubus_devices_cb user_callback = req->priv;

	if (user_callback) {
		user_callback(devices, num_devices);
	}
}

int ubus_devices(ubus_devices_cb callback)
{
	if (ubus_invoke(ubus_context, service_object_id, "devices", NULL, devices_cb, callback,
			UBUS_CALL_TIMEOUT)) {
		syslog(LOG_ERR, "Failed to call ubus devices method\n");
	}

	return 0;
}

enum { TOGGLE_RESPONSE, _TOGGLE_MAX };
static int parse_toggle_response_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy toggle_policy[_TOGGLE_MAX] = {
		[TOGGLE_RESPONSE] = { .name = "response", .type = BLOBMSG_TYPE_TABLE }
	};

	if (blobmsg_parse(toggle_policy, _TOGGLE_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) != 0) {
		syslog(LOG_ERR, "Parse toggle response failed\n");
		return 1;
	}

	if (attr[TOGGLE_RESPONSE] == NULL) {
		syslog(LOG_ERR, "No toggle response\n");
		return 1;
	}

	return 0;
}

enum { TOGGLE_RESP_RC, TOGGLE_RESP_MSG, _TOGGLE_RESP_MAX };
static int parse_toggle_response_body_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy response_policy[_TOGGLE_RESP_MAX] = {
		[TOGGLE_RESP_RC] = { .name = "response_code", .type = BLOBMSG_TYPE_INT32 },
		[TOGGLE_RESP_MSG] = { .name = "message", .type = BLOBMSG_TYPE_STRING }
	};

	if (blobmsg_parse(response_policy, _TOGGLE_RESP_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) !=
	    0) {
		syslog(LOG_ERR, "Parse toggle response body failed\n");
		return 1;
	}

	if (attr[TOGGLE_RESP_RC] == NULL) {
		syslog(LOG_ERR, "No toggle response code\n");
		return 1;
	}

	if (attr[TOGGLE_RESP_MSG] == NULL) {
		syslog(LOG_ERR, "No toggle response message\n");
		return 1;
	}

	return 0;
}

static void toggle_pin_on_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct ubus_toggle_result result;

	struct blob_attr *toggle_tb[_TOGGLE_MAX];
	if (parse_toggle_response_info(toggle_tb, msg))
		return;

	struct blob_attr *response_tb[_TOGGLE_RESP_MAX];
	if (parse_toggle_response_body_info(response_tb, toggle_tb[TOGGLE_RESPONSE]))
		return;

	result.response_code = blobmsg_get_u32(response_tb[TOGGLE_RESP_RC]);

	const char *message = blobmsg_get_string(response_tb[TOGGLE_RESP_MSG]);
	strlcpy(result.message, message, UBUS_ESP_MESSAGE_LENGTH);

	ubus_toggle_pin_cb user_callback = req->priv;

	if (user_callback) {
		user_callback(1, &result);
	}
}

static void toggle_pin_off_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct ubus_toggle_result result;

	struct blob_attr *toggle_tb[_TOGGLE_MAX];
	if (parse_toggle_response_info(toggle_tb, msg))
		return;

	struct blob_attr *response_tb[_TOGGLE_RESP_MAX];
	if (parse_toggle_response_body_info(response_tb, toggle_tb[TOGGLE_RESPONSE]))
		return;

	result.response_code = blobmsg_get_u32(response_tb[TOGGLE_RESP_RC]);

	const char *message = blobmsg_get_string(response_tb[TOGGLE_RESP_MSG]);
	strlcpy(result.message, message, UBUS_ESP_MESSAGE_LENGTH);

	ubus_toggle_pin_cb user_callback = req->priv;

	if (user_callback) {
		user_callback(0, &result);
	}
}

int ubus_enable_pin(const char *port, int pin, ubus_toggle_pin_cb callback)
{
	struct blob_buf b = {};
	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "port", port);
	blobmsg_add_u32(&b, "pin", (uint32_t)pin);

	if (ubus_invoke(ubus_context, service_object_id, "on", b.head, toggle_pin_on_cb, callback,
			UBUS_CALL_TIMEOUT)) {
		syslog(LOG_ERR, "Failed to call ubus enable pin method\n");
		blob_buf_free(&b);
		return 1;
	}

	blob_buf_free(&b);
	return 0;
}

int ubus_disable_pin(const char *port, int pin, ubus_toggle_pin_cb callback)
{
	struct blob_buf b = {};
	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "port", port);
	blobmsg_add_u32(&b, "pin", (uint32_t)pin);

	if (ubus_invoke(ubus_context, service_object_id, "off", b.head, toggle_pin_off_cb, callback,
			UBUS_CALL_TIMEOUT)) {
		syslog(LOG_ERR, "Failed to call ubus disable pin method\n");
		blob_buf_free(&b);
		return 1;
	}

	blob_buf_free(&b);
	return 0;
}

enum { GET_RESPONSE, _GET_MAX };
static int parse_get_response_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy get_policy[_GET_MAX] = {
		[GET_RESPONSE] = { .name = "response", .type = BLOBMSG_TYPE_TABLE }
	};

	if (blobmsg_parse(get_policy, _GET_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) != 0) {
		syslog(LOG_ERR, "Parse get response failed\n");
		return 1;
	}

	if (attr[GET_RESPONSE] == NULL) {
		syslog(LOG_ERR, "No get response\n");
		return 1;
	}

	return 0;
}

enum { GET_RESP_RC, GET_RESP_MSG, GET_RESP_DATA, _GET_RESP_MAX };
static int parse_get_response_body_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy response_policy[_GET_RESP_MAX] = {
		[GET_RESP_RC] = { .name = "response_code", .type = BLOBMSG_TYPE_INT32 },
		[GET_RESP_MSG] = { .name = "message", .type = BLOBMSG_TYPE_STRING },
		[GET_RESP_DATA] = { .name = "data", .type = BLOBMSG_TYPE_TABLE }
	};

	if (blobmsg_parse(response_policy, _GET_RESP_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) != 0) {
		syslog(LOG_ERR, "Parse get response body failed\n");
		return 1;
	}

	if (attr[GET_RESP_RC] == NULL) {
		syslog(LOG_ERR, "No get response code\n");
		return 1;
	}

	if (attr[GET_RESP_MSG] == NULL) {
		syslog(LOG_ERR, "No get response msg\n");
		return 1;
	}

	return 0;
}

enum { GET_DATA_HUMIDITY, GET_DATA_TEMPERATURE, _GET_DATA_MAX };
static int parse_get_data_info(struct blob_attr **attr, struct blob_attr *msg)
{
	static const struct blobmsg_policy data_policy[_GET_DATA_MAX] = {
		[GET_DATA_HUMIDITY] = { .name = "humidity", .type = BLOBMSG_TYPE_INT32 },
		[GET_DATA_TEMPERATURE] = { .name = "temperature", .type = BLOBMSG_TYPE_DOUBLE }
	};

	if (blobmsg_parse(data_policy, _GET_DATA_MAX, attr, blobmsg_data(msg), blobmsg_len(msg)) != 0) {
		syslog(LOG_ERR, "Parse get data failed\n");
		return 1;
	}

	if (attr[GET_DATA_HUMIDITY] == NULL) {
		syslog(LOG_ERR, "No get data humidity\n");
		return 1;
	}

	if (attr[GET_DATA_TEMPERATURE] == NULL) {
		syslog(LOG_ERR, "No get data temperature\n");
		return 1;
	}

	return 0;
}

static void get_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct ubus_get_result result;

	struct blob_attr *get_tb[_GET_MAX];
	if (parse_get_response_info(get_tb, msg))
		return;

	struct blob_attr *response_tb[_GET_RESP_MAX];
	if (parse_get_response_body_info(response_tb, get_tb[GET_RESPONSE]))
		return;

	result.response_code = blobmsg_get_u32(response_tb[GET_RESP_RC]);
	const char *message = blobmsg_get_string(response_tb[GET_RESP_MSG]);
	strlcpy(result.message, message, UBUS_ESP_MESSAGE_LENGTH);

	result.has_data = 0;
	if (response_tb[GET_RESP_DATA] != NULL) {
		struct blob_attr *data_tb[_GET_DATA_MAX];
		if (parse_get_data_info(data_tb, response_tb[GET_RESP_DATA]) == 0) {
			result.has_data = 1;
			result.humidity = blobmsg_get_u32(data_tb[GET_DATA_HUMIDITY]);
			result.temperature = blobmsg_get_double(data_tb[GET_DATA_TEMPERATURE]);
		}
	}

	ubus_get_cb user_callback = req->priv;

	if (user_callback) {
		user_callback(&result);
	}
}

int ubus_get(const char *port, int pin, const char *model, const char *sensor, ubus_get_cb callback)
{
	struct blob_buf b = {};
	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "port", port);
	blobmsg_add_u32(&b, "pin", (unsigned int)pin);
	blobmsg_add_string(&b, "model", model);
	blobmsg_add_string(&b, "sensor", sensor);

	if (ubus_invoke(ubus_context, service_object_id, "get", b.head, get_cb, callback,
			UBUS_CALL_TIMEOUT)) {
		syslog(LOG_ERR, "Failed to invoke ubus call to my_esp_controller get method\n");
		blob_buf_free(&b);
		return 1;
	}

	blob_buf_free(&b);
	return 0;
}