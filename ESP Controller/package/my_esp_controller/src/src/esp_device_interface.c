

// Abstraction for libserialport

#include <libserialport.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cJSON.h>

#include "esp_device_interface.h"
#include "util.h"

#define TARGET_VENDOR_ID 0x10C4
#define TARGET_PRODUCT_ID 0xEA60

#define TRX_TIMEOUT 1500

static int sp_check(enum sp_return result);

static int write_to_esp_device(struct sp_port *port, const char *str);
static int write_and_read_from_esp_device(struct sp_port *port, const char *str, cJSON **json_response);

int get_esp_device_info(struct esp_device_info *device_info, int num_requested_device_infos)
{
	struct sp_port **port_list;
	enum sp_return result = sp_list_ports(&port_list);
	if (result != SP_OK) {
		LU_PR_ERR("Failed listing ports");
		return -1;
	}

	int i;
	int esp_device_count = 0;
	for (i = 0; port_list[i] != NULL && esp_device_count < num_requested_device_infos; i++) {
		struct sp_port *port = port_list[i];

		int vid, pid;
		sp_get_port_usb_vid_pid(port, &vid, &pid);

		if (vid == TARGET_VENDOR_ID && pid == TARGET_PRODUCT_ID) {
			struct esp_device_info *dev_info = &device_info[esp_device_count];
			dev_info->port_name = strdup(sp_get_port_name(port));
			dev_info->vendor_id = vid;
			dev_info->product_id = pid;

			esp_device_count++;
		}
	}

	sp_free_port_list(port_list);

	return esp_device_count;
}

void *get_esp_device_by_port_name(const char *port_name)
{
	struct sp_port *ret;
	sp_check(sp_get_port_by_name(port_name, &ret));

	if (ret) {
		int vid, pid;
		sp_check(sp_get_port_usb_vid_pid(ret, &vid, &pid));

		if (vid == TARGET_VENDOR_ID && pid == TARGET_PRODUCT_ID) {
			return ret;
		}

		sp_free_port(ret);
	}

	return NULL;
}

void free_esp_device(void *dev_handle)
{
	struct sp_port *p = dev_handle;
	sp_free_port(p);
}

int esp_device_toggle_pin(void *dev_handle, int pin, int enable, cJSON **json_response)
{
	struct sp_port *port = dev_handle;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "action", enable ? "on" : "off");
	cJSON_AddNumberToObject(root, "pin", pin);

	char *json_string = cJSON_PrintUnformatted(root);

	int status = write_and_read_from_esp_device(port, json_string, json_response);

	cJSON_free(json_string);
	cJSON_Delete(root);

	if (status != 0) {
		LU_PR_ERR("Failed getting data from esp device");
		return status;
	}

	return 0;
}

int esp_device_get_data(void *dev_handle, int pin, const char *model, const char *sensor, cJSON **json_response)
{
	struct sp_port *port = dev_handle;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "action", "get");
	cJSON_AddNumberToObject(root, "pin", pin);
	cJSON_AddStringToObject(root, "model", model);
	cJSON_AddStringToObject(root, "sensor", sensor);

	char *json_string = cJSON_PrintUnformatted(root);

	int status = write_and_read_from_esp_device(port, json_string, json_response);

	cJSON_free(json_string);
	cJSON_Delete(root);

	if (status != 0) {
		LU_PR_ERR("Failed getting data from esp device");
		return status;
	}

	return 0;
}

// Non blocking write to a device. Send and forget
static int write_to_esp_device(struct sp_port *port, const char *str)
{
	if (sp_check(sp_open(port, SP_MODE_WRITE)) != SP_OK) {
		LU_PR_ERR("Failed opening device file");
		goto exit_error;
	}

	int size = strlen(str);
	int bytes_written = sp_nonblocking_write(port, str, size);
	if (bytes_written < 0) {
		sp_check(-bytes_written);
		LU_PR_ERR("Failed to write to esp device");
		goto close_and_exit_error;
	}
	LU_PR_INFO("Bytes written %d, expected to write %d", bytes_written, size);

	sp_close(port);
	return 0;
close_and_exit_error:
	sp_close(port);
exit_error:
	return -1;
}

// Blocking write and read from a device.
static int write_and_read_from_esp_device(struct sp_port *port, const char *str, cJSON **json_response)
{
	const int buf_out_size = 1024;
	char buf_out[buf_out_size];

	if (sp_check(sp_open(port, SP_MODE_READ_WRITE)) != SP_OK) {
		LU_PR_ERR("Failed opening device file");
		goto exit_error;
	}

	int size = strlen(str);
	int bytes_written = sp_blocking_write(port, str, size, TRX_TIMEOUT);
	if (bytes_written < 0) {
		sp_check(-bytes_written);
		LU_PR_ERR("Failed to write to esp device");
		goto close_and_exit_error;
	}
	//LU_PR_INFO("Bytes written %d, expected to write %d", bytes_written, size);
	//LU_PR_INFO("Written string: %s", str);

	sp_check(sp_drain(port));

	int buf_out_size_m1 = buf_out_size - 1; // Keep one slot for null term
	int bytes_received = sp_blocking_read(port, buf_out, buf_out_size_m1, TRX_TIMEOUT);
	if (bytes_received < 0) {
		sp_check(-bytes_received);
		LU_PR_ERR("Failed receiving bytes");
		goto close_and_exit_error;
	}

	// Shouldn't happen but just in case
	if (bytes_received >= buf_out_size) {
		LU_PR_ERR("Bytes received have overflown the buffer!! %d/%d", buf_out_size_m1, bytes_received);
		goto close_and_exit_error;
	}

	buf_out[bytes_received] = 0;

	//LU_PR_INFO("Bytes read %d", bytes_received);
	//LU_PR_INFO("String received: %s", buf_out);

	// Attempt to parse to cJSON
	*json_response = cJSON_Parse(buf_out);
	if (*json_response == NULL) {
		LU_PR_ERR("Received string cannot be parsed to json object");
		goto close_and_exit_error;
	}

	sp_close(port);
	return 0;
close_and_exit_error:
	sp_close(port);
exit_error:
	return -1;
}

static int sp_check(enum sp_return result)
{
	char *error_message;
	switch (result) {
	case SP_ERR_ARG:
		printf("Error: Invalid argument.\n");
		break;
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		printf("Error: Failed: %s\n", error_message);
		sp_free_error_message(error_message);
		break;
	case SP_ERR_SUPP:
		printf("Error: Not supported.\n");
		break;
	case SP_ERR_MEM:
		printf("Error: Couldn't allocate memory.\n");
		break;
	case SP_OK:
	default:
		break;
	}

	return result;
}
