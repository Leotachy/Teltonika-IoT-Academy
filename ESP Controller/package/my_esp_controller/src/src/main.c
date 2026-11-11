

#include <libserialport.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "esp_device_interface.h"
#include "ubus_interface.h"
#include "util.h"

static volatile sig_atomic_t quit = 0;
static void handle_signal(int sig);

int main(int argc, const char **argv)
{
	struct sigaction sa = {};

	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	int status;

	if (init_ubus_client() == 0) {
		while (quit == 0) {
			ubus_process();

			sleep(5);
		}
	}

	destroy_ubus_client();
}

static void handle_signal(int sig)
{
	(void)sig;
	quit = 1;
}
