
#ifndef __TUYA_INTERFACE_H
#define __TUYA_INTERFACE_H

// tuya headers
#include <tuyalink_core.h>
#include <tuya_endpoint.h>

int tuya_client_loop();
int init_tuya_client(const char *dev_id, const char *dev_secret, const char *prod_id);
void deinit_tuya_client();

// Returns non zero if the client is currently connected.
int is_tuya_client_connected();
// Returns non zero if a disconnection occured.
int is_tuya_client_disconnected();

// Warning: this function will free outputParams!
int send_tuya_action_response(const char *msg_id, const char *action_code, int rc, cJSON *outputParams);

#endif