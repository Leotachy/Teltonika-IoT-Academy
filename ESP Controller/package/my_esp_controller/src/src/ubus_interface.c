

// Abstraction for ubus

#include <libubox/blob.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <libubus.h>
#include <stdio.h>
#include <ubusmsg.h>
#include <unistd.h>

#include "esp_device_interface.h"
#include "ubus_interface.h"
#include "util.h"

static struct ubus_context *ubus_context = NULL;

static int ubusm_devices(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
			 const char *method, struct blob_attr *msg);

// no args

static int ubusm_on(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		    const char *method, struct blob_attr *msg);

enum { UBUSM_ON_PORT, UBUSM_ON_PIN, __UBUSM_ON_MAX };

static const struct blobmsg_policy ubusm_on_policy[__UBUSM_ON_MAX] = {
	[UBUSM_ON_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[UBUSM_ON_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};

static int ubusm_off(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		     const char *method, struct blob_attr *msg);

enum { UBUSM_OFF_PORT, UBUSM_OFF_PIN, __UBUSM_OFF_MAX };

static const struct blobmsg_policy ubusm_off_policy[__UBUSM_OFF_MAX] = {
	[UBUSM_OFF_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[UBUSM_OFF_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
};

static int ubusm_get(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		     const char *method, struct blob_attr *msg);

enum { UBUSM_GET_PORT, UBUSM_GET_PIN, UBUSM_GET_MODEL, UBUSM_GET_SENSOR, __UBUSM_GET_MAX };

static const struct blobmsg_policy ubusm_get_policy[__UBUSM_GET_MAX] = {
	[UBUSM_GET_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[UBUSM_GET_PIN] = { .name = "pin", .type = BLOBMSG_TYPE_INT32 },
	[UBUSM_GET_MODEL] = { .name = "model", .type = BLOBMSG_TYPE_STRING },
	[UBUSM_GET_SENSOR] = { .name = "sensor", .type = BLOBMSG_TYPE_STRING },
};

static const struct ubus_method ubus_methods[] = {
	UBUS_METHOD_NOARG("devices", ubusm_devices),
	UBUS_METHOD("on", ubusm_on, ubusm_on_policy),
	UBUS_METHOD("off", ubusm_off, ubusm_off_policy),
	UBUS_METHOD("get", ubusm_get, ubusm_get_policy),
};

static struct ubus_object_type ubus_object_type = UBUS_OBJECT_TYPE("my_esp_controller", ubus_methods);

static struct ubus_object ubus_object = {
	.name = "my_esp_controller",
	.type = &ubus_object_type,
	.methods = ubus_methods,
	.n_methods = ARRAY_SIZE(ubus_methods),
};

int init_ubus_client(void)
{
	ubus_context = ubus_connect(NULL);

	if (ubus_context == NULL) {
		LU_PR_ERR("Failed connecting to ubus server!");
		return 1;
	}

	int ubus_status = ubus_add_object(ubus_context, &ubus_object);
	if (ubus_status) {
		LU_PR_ERR("Failed adding ubus object. Error code: %d", ubus_status);
		return ubus_status;
	}

	LU_PR_INFO("Ubus client initialized successfully");
	return 0;
}

void destroy_ubus_client(void)
{
	if (ubus_context) {
		ubus_free(ubus_context);
		ubus_context = NULL;
	}
}

void ubus_process(void)
{
	ubus_handle_event(ubus_context);
}

// UBUS methods

static int ubusm_devices(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
			 const char *method, struct blob_attr *msg)
{
	const int n = 20;
	struct esp_device_info device_infos[n];
	int num_device_infos = get_esp_device_info(device_infos, ARRAY_SIZE(device_infos));

	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	void *arr = blobmsg_open_array(&b, "devices");
	for (int i = 0; i < num_device_infos; i++) {
		struct esp_device_info *dev_info = &device_infos[i];

		void *table = blobmsg_open_table(&b, NULL);

		blobmsg_add_string(&b, "port", dev_info->port_name);

		free(dev_info->port_name);
		dev_info->port_name = NULL;

		const int hex_buf_size = 64;
		char hex_buf[hex_buf_size];
		// convert to hex
		to_hex_string(hex_buf, hex_buf_size, dev_info->vendor_id);
		blobmsg_add_string(&b, "vendor_id", hex_buf);

		to_hex_string(hex_buf, hex_buf_size, dev_info->product_id);
		blobmsg_add_string(&b, "product_id", hex_buf);

		blobmsg_close_table(&b, table);
	}
	blobmsg_close_array(&b, arr);

	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	return 0;
}

static int ubusm_on_parse_params(struct blob_attr *msg, const char **port, int *pin)
{
	struct blob_attr *tb[__UBUSM_ON_MAX];
	blobmsg_parse(ubusm_on_policy, __UBUSM_ON_MAX, tb, blob_data(msg), blob_len(msg));
	if (tb[UBUSM_ON_PORT] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (tb[UBUSM_ON_PIN] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;

	const char *p = blobmsg_get_string(tb[UBUSM_ON_PORT]);
	if (port == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;

	int pi = blobmsg_get_u32(tb[UBUSM_ON_PIN]);

	*port = p;
	*pin = pi;
	return UBUS_STATUS_OK;
}

static int ubusm_on(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		    const char *method, struct blob_attr *msg)
{
	const char *port;
	int pin;
	int parse_status = ubusm_on_parse_params(msg, &port, &pin);
	if (parse_status)
		return parse_status;

	// Get esp device index

	void *p = get_esp_device_by_port_name(port);
	if (p == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT; // No esp device with such port

	cJSON *json_response = NULL;
	int status = esp_device_toggle_pin(p, pin, 1, &json_response);
	free_esp_device(p);

	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	void *table_cookie = blobmsg_open_table(&b, "response");

	// Parse out data from json
	if (json_response) {
		cJSON *response_code_json = cJSON_GetObjectItemCaseSensitive(json_response, "rc");
		if (response_code_json) {
			int rc = response_code_json->valueint;
			blobmsg_add_u32(&b, "response_code", rc);
		}

		cJSON *msg_json = cJSON_GetObjectItemCaseSensitive(json_response, "msg");
		if (msg_json) {
			const char *msg_string = cJSON_GetStringValue(msg_json);
			blobmsg_add_string(&b, "message", msg_string);
		}

		cJSON_Delete(json_response);
	}

	blobmsg_close_table(&b, table_cookie);
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	return 0;
}

static int ubusm_off_parse_params(struct blob_attr *msg, const char **port, int *pin)
{
	struct blob_attr *tb[__UBUSM_OFF_MAX];
	blobmsg_parse(ubusm_off_policy, __UBUSM_OFF_MAX, tb, blob_data(msg), blob_len(msg));
	if (tb[UBUSM_OFF_PORT] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (tb[UBUSM_OFF_PIN] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;

	const char *p = blobmsg_get_string(tb[UBUSM_OFF_PORT]);
	if (port == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;

	int pi = blobmsg_get_u32(tb[UBUSM_OFF_PIN]);

	*port = p;
	*pin = pi;
	return UBUS_STATUS_OK;
}

static int ubusm_off(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		     const char *method, struct blob_attr *msg)
{
	const char *port;
	int pin;
	int parse_status = ubusm_off_parse_params(msg, &port, &pin);
	if (parse_status)
		return parse_status;

	// Get esp device index
	void *p = get_esp_device_by_port_name(port);
	if (p == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT; // No esp device with such port

	cJSON *json_response = NULL;
	int status = esp_device_toggle_pin(p, pin, 0, &json_response);
	free_esp_device(p);

	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	void *table_cookie = blobmsg_open_table(&b, "response");

	// Parse out data from json
	if (json_response) {
		cJSON *response_code_json = cJSON_GetObjectItemCaseSensitive(json_response, "rc");
		if (response_code_json) {
			int rc = response_code_json->valueint;
			blobmsg_add_u32(&b, "response_code", rc);
		}

		cJSON *msg_json = cJSON_GetObjectItemCaseSensitive(json_response, "msg");
		if (msg_json) {
			const char *msg_string = cJSON_GetStringValue(msg_json);
			blobmsg_add_string(&b, "message", msg_string);
		}

		cJSON_Delete(json_response);
	}

	blobmsg_close_table(&b, table_cookie);
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	return 0;
}

static int ubusm_get_parse_params(struct blob_attr *msg, const char **port, int *pin, const char **model,
				  const char **sensor)
{
	struct blob_attr *tb[__UBUSM_GET_MAX];
	blobmsg_parse(ubusm_get_policy, __UBUSM_GET_MAX, tb, blob_data(msg), blob_len(msg));
	if (tb[UBUSM_GET_PORT] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (tb[UBUSM_GET_PIN] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (tb[UBUSM_GET_MODEL] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (tb[UBUSM_GET_SENSOR] == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;

	const char *p = blobmsg_get_string(tb[UBUSM_GET_PORT]);
	if (p == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	int pi = blobmsg_get_u32(tb[UBUSM_GET_PIN]);
	const char *mdl = blobmsg_get_string(tb[UBUSM_GET_MODEL]);
	if (mdl == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;
	const char *snsr = blobmsg_get_string(tb[UBUSM_GET_SENSOR]);
	if (snsr == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT;

	*port = p;
	*pin = pi;
	*model = mdl;
	*sensor = snsr;
	return UBUS_STATUS_OK;
}

static int ubusm_get(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req,
		     const char *method, struct blob_attr *msg)
{
	const char *port;
	int pin;
	const char *model;
	const char *sensor;
	int parse_status = ubusm_get_parse_params(msg, &port, &pin, &model, &sensor);
	if (parse_status)
		return parse_status;

	// Get esp device index
	void *p = get_esp_device_by_port_name(port);
	if (p == NULL)
		return UBUS_STATUS_INVALID_ARGUMENT; // No esp device with such port

	cJSON *json_response = NULL;
	int status = esp_device_get_data(p, pin, model, sensor, &json_response);
	free_esp_device(p);

	// Construct return json
	struct blob_buf b = {};
	blob_buf_init(&b, 0);

	void *table_cookie = blobmsg_open_table(&b, "response");

	if (json_response) {
		cJSON *response_code_json = cJSON_GetObjectItemCaseSensitive(json_response, "rc");
		if (response_code_json) {
			int rc = response_code_json->valueint;
			blobmsg_add_u32(&b, "response_code", rc);
		}

		cJSON *msg_json = cJSON_GetObjectItemCaseSensitive(json_response, "msg");
		if (msg_json) {
			const char *msg_string = cJSON_GetStringValue(msg_json);
			blobmsg_add_string(&b, "message", msg_string);
		}

		cJSON *data_cjson = cJSON_GetObjectItemCaseSensitive(json_response, "data");
		if (data_cjson) {
			void *table_data_cookie = blobmsg_open_table(&b, "data");

			cJSON *humidity_cjson = cJSON_GetObjectItemCaseSensitive(data_cjson, "humidity");
			if (humidity_cjson) {
				int humidity = humidity_cjson->valueint;
				blobmsg_add_u32(&b, "humidity", humidity);
			}

			cJSON *temperature_cjson = cJSON_GetObjectItemCaseSensitive(data_cjson, "temperature");
			if (temperature_cjson) {
				double temperature = temperature_cjson->valuedouble;
				blobmsg_add_double(&b, "temperature", temperature);
			}

			blobmsg_close_table(&b, table_data_cookie);
		}

		cJSON_Delete(json_response);
	}

	blobmsg_close_table(&b, table_cookie);

	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	return 0;
}
