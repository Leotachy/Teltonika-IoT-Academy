#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <curl/curl.h>
#include <cJSON/cJSON.h>

#define APP_NAME "luipgeo"
#define PARAMS_STRING_LENGTH 256
struct parse_options_params {
	char api_key[PARAMS_STRING_LENGTH];
	char ip_address[PARAMS_STRING_LENGTH];
};
static void print_usage(void);
static void read_options(int argc, char *const *argv, struct parse_options_params *params);
static size_t api_response_callback(void *contents, size_t size, size_t nmemb, void *userp);
static int do_and_print_api_request(const char *api_key, const char *ip_address);

int main(int argc, char *const *argv)
{
	struct parse_options_params params;
	read_options(argc, argv, &params);

	do_and_print_api_request(params.api_key, params.ip_address[0] ? params.ip_address : NULL);
}

static void print_usage(void)
{
	puts("Usage: " APP_NAME " -k <api_key> [-i <ip_address>]");
}

static void read_options(int argc, char *const *argv, struct parse_options_params *params)
{
	// Defaults
	params->api_key[0] = 0;
	params->ip_address[0] = 0;

	const struct option long_options[] = { { "api-key", required_argument, 0, 'k' },
					       { "ip_address", required_argument, 0, 'i' },
					       { "help", no_argument, 0, 'H' },
					       { 0, 0, 0, 0 } };

	int opt = 0;
	int long_opt = -1;
	while ((opt = getopt_long(argc, argv, "k:i:H", long_options, &long_opt)) != -1) {
		switch (opt) {
		case 'k':
			strlcpy(params->api_key, optarg, PARAMS_STRING_LENGTH);
			break;
		case 'i':
			strlcpy(params->ip_address, optarg, PARAMS_STRING_LENGTH);
			break;
		case 'H':
			print_usage();
			exit(EXIT_SUCCESS);
			break;
		default:
			print_usage();
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (params->api_key[0] == 0) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}

/* holder for curl fetch */
struct curl_fetch_st {
	char *payload;
	size_t size;
};

/* callback for curl fetch */
static size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb; /* calculate buffer size */
	struct curl_fetch_st *p = (struct curl_fetch_st *)userp; /* cast pointer to fetch struct */

	/* expand buffer */
	p->payload = (char *)realloc(p->payload, p->size + realsize + 1);

	/* check buffer */
	if (p->payload == NULL) {
		/* this isn't good */
		fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
		/* free buffer */
		free(p->payload);
		/* return */
		return -1;
	}

	/* copy contents to buffer */
	memcpy(&(p->payload[p->size]), contents, realsize);

	/* set new buffer size */
	p->size += realsize;

	/* ensure null termination */
	p->payload[p->size] = 0;

	/* return size */
	return realsize;
}

/* fetch and return url body via curl */
static CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch)
{
	CURLcode rcode; /* curl result code */

	/* init payload */
	fetch->payload = (char *)calloc(1, sizeof(fetch->payload));

	/* check payload */
	if (fetch->payload == NULL) {
		/* log error */
		fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url");
		/* return error */
		return CURLE_FAILED_INIT;
	}

	/* init size */
	fetch->size = 0;

	curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(ch, CURLOPT_SSL_VERIFYHOST, 0);

	/* set url to fetch */
	curl_easy_setopt(ch, CURLOPT_URL, url);

	/* set calback function */
	curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

	/* pass fetch struct pointer */
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *)fetch);

	/* set default user agent */
	curl_easy_setopt(ch, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* set timeout */
	curl_easy_setopt(ch, CURLOPT_TIMEOUT, 5);

	/* enable location redirects */
	curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

	/* set maximum allowed redirects */
	curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);

	/* fetch the url */
	rcode = curl_easy_perform(ch);

	/* return */
	return rcode;
}

static int do_and_print_api_request(const char *api_key, const char *ip_address)
{
	CURL *curl;
	CURLcode result = curl_global_init(CURL_GLOBAL_NOTHING);
	if (result != CURLE_OK)
		return (int)result;

	curl = curl_easy_init();
	if (curl) {
		char buf[512];

		if (ip_address) {
			snprintf(buf, sizeof(buf),
				 "https://api.geoapify.com/v1/ipinfo?ip=%s&apiKey=%s", ip_address,
				 api_key);
		} else {
			snprintf(buf, sizeof(buf), "https://api.geoapify.com/v1/ipinfo?apiKey=%s",
				 api_key);
		}

		struct curl_fetch_st curl_fetch = { .payload = NULL, .size = 0 };
		result = curl_fetch_url(curl, buf, &curl_fetch);

		/* Check for errors */
		bool success = false;
		if (result == CURLE_OK) {
			cJSON *cjson = cJSON_Parse(curl_fetch.payload);

			cJSON *city_cjson = cJSON_GetObjectItemCaseSensitive(cjson, "city");
			cJSON *city_name_cjson =
				cJSON_GetObjectItemCaseSensitive(city_cjson, "name");
			const char *city_name = cJSON_GetStringValue(city_name_cjson);

			cJSON *country_cjson = cJSON_GetObjectItemCaseSensitive(cjson, "country");
			cJSON *country_name_cjson =
				cJSON_GetObjectItemCaseSensitive(country_cjson, "name");
			const char *country_name = cJSON_GetStringValue(country_name_cjson);

			if (city_name && country_name) {
				printf("Location: %s, %s\n", city_name, country_name);
				success = true;
			}

			cJSON_free(cjson);

		} else {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(result));
		}

		if (success == false) {
			printf("Failed getting location from the ip address\n");
		}

		if (curl_fetch.payload) {
			free(curl_fetch.payload);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return 0;
}
