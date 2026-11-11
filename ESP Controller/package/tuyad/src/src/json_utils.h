
#ifndef __TUYA_UTILS_H
#define __TUYA_UTILS_H

#include <cJSON.h>

int get_valuestr_from_json_key(cJSON *json, const char *key, const char **ret);
int get_valueint_from_json_key(cJSON *json, const char *key, int *ret);

#endif