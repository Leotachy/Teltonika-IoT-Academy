

// Teltonika IoT akademija
// Report system status to Tuya cloud
// Lukas Andrijauskas 22-Sep-2026

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#include "tuya_interface.h"
#include "ubus_interface.h"

#define APP_NAME "tuyad"
#define REPORT_STATUS_INTERVAL 15
#define GET_SYSTEM_STATUS_MAX_CONSEQUENT_FAILURES 5
#define PARAMS_STRING_LENGTH 256

struct app_options {
	char device_id[PARAMS_STRING_LENGTH];
	char device_secret[PARAMS_STRING_LENGTH];
	char product_id[PARAMS_STRING_LENGTH];
};

static volatile int quit = 0;

static void process_loop(void);

static void print_usage(void);
static void handle_signals(int signal);
static void read_options(int argc, char *const *argv, struct app_options *params);

int main(int argc, char *const *argv)
{
	signal(SIGINT, handle_signals);
	signal(SIGABRT, handle_signals);
	signal(SIGTERM, handle_signals);
	signal(SIGQUIT, handle_signals);

	openlog(APP_NAME, 0, LOG_DAEMON);
	syslog(LOG_INFO, APP_NAME " started as daemon\n");

	struct app_options params;
	read_options(argc, argv, &params);

	if (init_ubus_client()) {
		syslog(LOG_ERR, "Failed to init ubus client\n");
		exit(EXIT_FAILURE);
	}

	if (init_tuya_client(params.device_id, params.device_secret, params.product_id) != 0) {
		syslog(LOG_ERR, "Failed to init tuya client\n");
		exit(EXIT_FAILURE);
	}

	process_loop();

	syslog(LOG_INFO, "Shutting down....\n");
	deinit_ubus_client();
	deinit_tuya_client();

	closelog();

	return 0;
}

static void process_loop(void)
{
	while (quit == 0) {
		int tuya_ret = tuya_client_loop();

		if (tuya_ret != 0) {
			// Tuya connection died, RIP
			syslog(LOG_ERR, "Tuya client loop returned %d. Quitting\n", tuya_ret);
			break;
		}

		if (is_tuya_client_connected()) {
		} else if (is_tuya_client_disconnected()) {
			syslog(LOG_INFO, "Tuya client disconnected. Quitting\n");
			break;
		}

		sleep(5);
	}
}

static void print_usage(void)
{
	puts("Usage: " APP_NAME " -d <device_id> -s <device_secret> -p <product_id> [-b]");
}

static void handle_signals(int signal)
{
	syslog(LOG_INFO, "Signal %d received\n", signal);
	quit = 1;
}

static void read_options(int argc, char *const *argv, struct app_options *params)
{
	// Defaults
	params->device_id[0] = 0;
	params->device_secret[0] = 0;
	params->product_id[0] = 0;

	const struct option long_options[] = { { "device-id", required_argument, 0, 'd' },
					       { "device-secret", required_argument, 0, 's' },
					       { "product-id", required_argument, 0, 'p' },
					       { "help", no_argument, 0, 'H' },
					       { 0, 0, 0, 0 } };

	int opt = 0;
	int long_opt = -1;
	while ((opt = getopt_long(argc, argv, "d:s:p:bH", long_options, &long_opt)) != -1) {
		switch (opt) {
		case 'd':
			strlcpy(params->device_id, optarg, PARAMS_STRING_LENGTH);
			break;
		case 's':
			strlcpy(params->device_secret, optarg, PARAMS_STRING_LENGTH);
			break;
		case 'p':
			strlcpy(params->product_id, optarg, PARAMS_STRING_LENGTH);
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

	if ((params->device_id[0] == 0 || params->device_secret[0] == 0 || params->product_id[0] == 0)) {
		print_usage();
		exit(EXIT_FAILURE);
	}
}
