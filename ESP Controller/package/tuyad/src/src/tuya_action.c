
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>

#include <cJSON.h>

#include "tuya_interface.h"
#include "json_utils.h"

#include "ubus_interface.h"

#define CURRENT_MSG_ID_BUF_SIZE 128
static char current_msg_id[CURRENT_MSG_ID_BUF_SIZE] = { 0 };

static void execute_test_action(const char *value)
{
#define TUYA_ACTION_LOG_FILE "/tmp/tuya_action.log"
	FILE *f = fopen(TUYA_ACTION_LOG_FILE, "a");
	if (f == NULL) {
		syslog(LOG_ERR, "Failed creating/opening file: " TUYA_ACTION_LOG_FILE "\n");
		return;
	}

	int written = fwrite(value, sizeof(char), strlen(value), f);
	if (written <= 0) {
		syslog(LOG_ERR, "Failed writing to file: " TUYA_ACTION_LOG_FILE "\n");
		goto close_and_exit;
	}

	written = fwrite("\n", sizeof(char), 1, f);
	if (written <= 0) {
		syslog(LOG_ERR, "Failed writing to file: " TUYA_ACTION_LOG_FILE "\n");
		goto close_and_exit;
	}

close_and_exit:
	fclose(f);
}

void execute_devices_action_cb(struct ubus_device_result *devices, int num_devices)
{
	cJSON *devices_json = cJSON_CreateObject();

	cJSON *array = cJSON_AddArrayToObject(devices_json, "devices");

	for (int i = 0; i < num_devices; i++) {
		cJSON *device_obj = cJSON_CreateObject();

		cJSON_AddStringToObject(device_obj, "port", devices[i].port);
		cJSON_AddStringToObject(device_obj, "vendor_id", devices[i].vendor_id);
		cJSON_AddStringToObject(device_obj, "product_id", devices[i].product_id);

		cJSON_AddItemToArray(array, device_obj);
	}

	send_tuya_action_response(current_msg_id, "devices", 0, devices_json);
}

static int execute_devices_action(void)
{
	int status = ubus_devices(execute_devices_action_cb);
	if (status) {
		syslog(LOG_ERR, "Ubus devices failed\n");
		return status;
	}

	return 0;
}

void execute_toggle_action_cb(int enable, struct ubus_toggle_result *result)
{
	cJSON *toggle_json = cJSON_CreateObject();

	cJSON_AddStringToObject(toggle_json, "message", result->message);
	send_tuya_action_response(current_msg_id, enable ? "on" : "off", result->response_code, toggle_json);
}

static int execute_enable_pin_action(const char *port, int pin)
{
	int status = ubus_enable_pin(port, pin, execute_toggle_action_cb);
	if (status) {
		syslog(LOG_ERR, "Ubus enable pin failed\n");
		return status;
	}

	return 0;
}

static int execute_disable_pin_action(const char *port, int pin)
{
	int status = ubus_disable_pin(port, pin, execute_toggle_action_cb);
	if (status) {
		syslog(LOG_ERR, "Ubus disable pin failed\n");
		return status;
	}

	return 0;
}

static void execute_get_action_cb(struct ubus_get_result *result)
{
	cJSON *get_json = cJSON_CreateObject();

	cJSON_AddStringToObject(get_json, "message", result->message);

	if (result->has_data) {
		cJSON *data_json = cJSON_AddObjectToObject(get_json, "sensor_data");

		cJSON_AddNumberToObject(data_json, "humidity", result->humidity);
		cJSON_AddNumberToObject(data_json, "temperature", result->temperature);
	}

	send_tuya_action_response(current_msg_id, "get", result->response_code, get_json);
}

static int execute_get_action(const char *port, int pin, const char *sensor, const char *model)
{
	int status = ubus_get(port, pin, sensor, model, execute_get_action_cb);
	if (status) {
		syslog(LOG_ERR, "Ubus get failed\n");
		return status;
	}

	return 0;
}

static int handle_action(const char *action_name, cJSON *input_params_json)
{
	if (strcmp(action_name, "action_test") == 0) {
		const char *text;

		if (get_valuestr_from_json_key(input_params_json, "text", &text) == 0)
			execute_test_action(text);
		else
			syslog(LOG_ERR, "Missing input parameters for 'action_test' action\n");

		return 1;
	}

	if (strcmp(action_name, "devices") == 0) {
		execute_devices_action();
		return 1;
	}
	if (strcmp(action_name, "on") == 0) {
		const char *port;
		int pin;

		if (get_valuestr_from_json_key(input_params_json, "port", &port) == 0 &&
		    get_valueint_from_json_key(input_params_json, "pin", &pin) == 0)
			execute_enable_pin_action(port, pin);
		else
			syslog(LOG_ERR, "Missing input parameters for 'on' action\n");

		return 1;
	}
	if (strcmp(action_name, "off") == 0) {
		const char *port;
		int pin;

		if (get_valuestr_from_json_key(input_params_json, "port", &port) == 0 &&
		    get_valueint_from_json_key(input_params_json, "pin", &pin) == 0)
			execute_disable_pin_action(port, pin);
		else
			syslog(LOG_ERR, "Missing input parameters for 'off' action\n");

		return 1;
	}
	if (strcmp(action_name, "get") == 0) {
		const char *port;
		int pin;
		const char *sensor;
		const char *model;

		if (get_valuestr_from_json_key(input_params_json, "port", &port) == 0 &&
		    get_valueint_from_json_key(input_params_json, "pin", &pin) == 0 &&
		    get_valuestr_from_json_key(input_params_json, "sensor", &sensor) == 0 &&
		    get_valuestr_from_json_key(input_params_json, "model", &model) == 0)
			execute_get_action(port, pin, sensor, model);
		else
			syslog(LOG_ERR, "Missing input parameters for 'get' action\n");

		return 1;
	}

	// We don't care about this message
	return 0;
}

void handle_action_request(char *msg_id, const char *str)
{
	if (current_msg_id[0]) {
		syslog(LOG_INFO, "Cannot execute action %s because we are already executing one\n", str);
		return;
	}

	strlcpy(current_msg_id, msg_id, CURRENT_MSG_ID_BUF_SIZE);
	syslog(LOG_INFO, "Current msg id: %s\n", current_msg_id);

	syslog(LOG_INFO, "Executing action: %s\n", str);

	cJSON *json = cJSON_Parse(str);
	if (json == NULL) {
		syslog(LOG_ERR, "Action message data string couldn't be parsed to JSON!! Message:\n%s\n",
		       str);
		return;
	}

	const char *action_code;

	if (get_valuestr_from_json_key(json, "actionCode", &action_code) == 0) {
		cJSON *input_params_json = cJSON_GetObjectItemCaseSensitive(json, "inputParams");

		if (input_params_json) {
			handle_action(action_code, input_params_json);
		} else {
			syslog(LOG_ERR, "Action message data string does not contain inputParams\n");
		}
	}

	cJSON_Delete(json);
	current_msg_id[0] = 0;
}