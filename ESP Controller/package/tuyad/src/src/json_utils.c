

#include <stdio.h>
#include <string.h>

#include <cJSON.h>

#include "json_utils.h"

int get_valuestr_from_json_key(cJSON *json, const char *key, const char **ret)
{
	if (json == NULL || key == NULL)
		return 1;

	cJSON *key_json = cJSON_GetObjectItemCaseSensitive(json, key);
	if (key_json) {
		if (cJSON_IsString(key_json)) {
			*ret = key_json->valuestring;
			return 0;
		}
	}

	return 1;
}

int get_valueint_from_json_key(cJSON *json, const char *key, int *ret)
{
	if (json == NULL || key == NULL)
		return 1;

	cJSON *key_json = cJSON_GetObjectItemCaseSensitive(json, key);
	if (key_json) {
		if (cJSON_IsString(key_json) == 0) {
			*ret = key_json->valueint;
			return 0;
		}
	}

	return 1;
}