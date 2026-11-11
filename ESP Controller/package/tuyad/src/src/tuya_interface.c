
// Tuya client funcs

#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <cJSON.h>

#include "tuya_interface.h"
#include "tuya_cacert.h"
#include "tuya_action.h"
#include "json_utils.h"

static tuya_mqtt_context_t tuya_client_instance;
static int tuya_connected = 0;
static int tuya_disconnected = 0;

static const char *device_id = NULL;
static const char *device_secret = NULL;
static const char *product_id = NULL;

#define MAX_TUYA_CONNECTION_ATTEMPTS 5

static void on_tuya_client_connected(tuya_mqtt_context_t *client, void *userdata)
{
	syslog(LOG_INFO, "Connected to tuya\n");
	tuya_connected = 1;
}

static void on_tuya_client_disconnected(tuya_mqtt_context_t *client, void *userdata)
{
	syslog(LOG_INFO, "Disconnected from tuya\n");
	tuya_connected = 0;
	tuya_disconnected = 1;
}

void on_tuya_client_messages(tuya_mqtt_context_t *context, void *user_data, const tuyalink_message_t *msg)
{
	syslog(LOG_INFO, "on message id: %s, type: %d, code: %d\n", msg->msgid, msg->type, msg->code);
	switch (msg->type) {
	case THING_TYPE_ACTION_EXECUTE: {
		handle_action_request(msg->msgid, msg->data_string);
		break;
	}

	default:
		break;
	}
}

int tuya_client_loop()
{
	return tuya_mqtt_loop(&tuya_client_instance);
}

int init_tuya_client(const char *dev_id, const char *dev_secret, const char *prod_id)
{
	device_id = dev_id;
	device_secret = dev_secret;
	product_id = prod_id;

	int ret = tuya_mqtt_init(&tuya_client_instance,
				 &(const tuya_mqtt_config_t){ .host = "m1.tuyacn.com",
							      .port = 8883,
							      .cacert = tuya_cacert_pem,
							      .cacert_len = sizeof(tuya_cacert_pem),
							      .device_id = device_id,
							      .device_secret = device_secret,
							      .keepalive = 60,
							      .timeout_ms = 2000,
							      .on_connected = on_tuya_client_connected,
							      .on_disconnect = on_tuya_client_disconnected,
							      .on_messages = on_tuya_client_messages });

	if (ret) {
		syslog(LOG_ERR, "Failed to initialize tuya client\n");
		return ret;
	}

	int fail = 0;

	while ((ret = tuya_mqtt_connect(&tuya_client_instance)) != 0) {
		fail++;
		if (fail >= MAX_TUYA_CONNECTION_ATTEMPTS) {
			syslog(LOG_ERR, "Failed to connect tuya client. Giving up\n");
			return ret;
		}

		syslog(LOG_WARNING, "Failed to connect tuya client. Trying again. Attempt %d/%d....\n", fail,
		       MAX_TUYA_CONNECTION_ATTEMPTS);
		sleep(3);
	}

	return ret;
}

void deinit_tuya_client()
{
	if (is_tuya_client_connected()) {
		tuya_mqtt_disconnect(&tuya_client_instance);
	}

	tuya_mqtt_deinit(&tuya_client_instance);
}

int is_tuya_client_connected()
{
	return tuya_connected;
}

int is_tuya_client_disconnected()
{
	return tuya_disconnected;
}

int send_tuya_action_response(const char *msg_id, const char *action_code, int rc, cJSON *outputParams)
{
	cJSON *json = cJSON_CreateObject();

	cJSON_AddStringToObject(json, "actionCode", action_code);

	cJSON_AddItemToObject(json, "outputParams", outputParams);

	// We are done constructing the JSON, yaay
	// Now send the response

	char *str = cJSON_PrintUnformatted(json);

	tuyalink_message_t message = { .type = THING_TYPE_ACTION_EXECUTE_RSP,
				       .device_id = (char *)device_id,
				       .msgid = (char *)msg_id,
				       .code = rc,
				       .data_string = str };

	int tuyalink_message_send(tuya_mqtt_context_t * context, tuyalink_message_t * message);
	int res = tuyalink_message_send(&tuya_client_instance, &message);

	if (res) {
		syslog(LOG_ERR, "Failed sending system status JSON data\n");
	}

	cJSON_free(str);
	cJSON_Delete(json);
	return res;
}