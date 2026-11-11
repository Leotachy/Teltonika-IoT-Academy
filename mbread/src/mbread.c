

// Lukas Andrijauskas
// Teltonika IoT academy
// Nov 4th, 2026

// A manual modbus client test

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "modbus.h"

struct mbread_options {
	char ip_address[16];
	uint16_t port;
	uint16_t reg_address;
	uint16_t value;
	uint8_t count;

	// Validation bits
	uint8_t ip_address_valid : 1;
	uint8_t port_valid : 1;
	uint8_t reg_address_valid : 1;
	uint8_t value_valid : 1;
	uint8_t count_valid : 1;

	uint8_t write_mode;
};

static int read_options(int argc, char *const *argv, struct mbread_options *opts);

int main(int argc, char *const *argv)
{
	struct mbread_options opts = { 0 };
	if (read_options(argc, argv, &opts))
		goto exit_failure;

	mb_client_context_t context;
	if (mb_client_context_init(&context, opts.ip_address, opts.port)) {
		puts("Failed initializing modbus client context");
		goto exit_failure;
	}

	int transaction_status = 0;
	if (opts.write_mode)
		transaction_status = mb_transaction_write_reg(&context, opts.reg_address, opts.value);
	else
		transaction_status = mb_transaction_read_seq_reg(&context, opts.reg_address, opts.count);

	if (transaction_status) {
		puts("Modbus transaction failed");
		goto cleanup_and_exit_failure;
	}

	mb_client_context_destroy(&context);
	return 0;

cleanup_and_exit_failure:
	mb_client_context_destroy(&context);
exit_failure:
	return 1;
}

static void print_usage(void)
{
	puts("Usage: -i <ip_address> -p <port> | If read mode: -a <mb_register_address> -c "
	     "<mb_num_registers>, If write mode: -w -v <value>");
}

static int read_options(int argc, char *const *argv, struct mbread_options *opts)
{
	int option;
	char *endptr;
	while ((option = getopt(argc, argv, "i:p:a:c:wv:")) != -1) {
		switch (option) {
		case 'i':
			strlcpy(opts->ip_address, optarg, sizeof(opts->ip_address));

			opts->ip_address_valid = 1;
			break;
		case 'p': {
			long port = strtol(optarg, &endptr, 10);
			if (port < 1 || port > 65535 || endptr == optarg) {
				puts("Port must be in range 1-65535");
				return 1;
			}

			opts->port = (__uint16_t)port;
			opts->port_valid = 1;
			break;
		}
		case 'a': {
			long address = strtol(optarg, &endptr, 10);
			if (address < 0 || address > 65535 || endptr == optarg) {
				puts("Modbus register address must be in range 0-65535");
				return 1;
			}

			opts->reg_address = (__uint16_t)address;
			opts->reg_address_valid = 1;
			break;
		}
		case 'c': {
			long count = strtol(optarg, &endptr, 10);
			if (count < 1 || count > 125 || endptr == optarg) {
				puts("Modbus num registers to read must be in range 1-125");
				return 1;
			}

			opts->count = (__uint8_t)count;
			opts->count_valid = 1;
			break;
		}
		case 'w': {
			opts->write_mode = 1;
			break;
		}
		case 'v': {
			long value = strtol(optarg, &endptr, 10);
			if (value < 0 || value > __UINT16_MAX__ || endptr == optarg) {
				puts("Value must be in range of 0-65535");
				return 1;
			}

			opts->value = value;
			opts->value_valid = 1;
			break;
		}

		default:
			print_usage();
			return 1;
		}
	}

	if (opts->write_mode) {
		if (!opts->ip_address_valid || !opts->port_valid || !opts->reg_address_valid || !opts->value_valid) {
			print_usage();
			return 1;
		}
	} else {
		if (!opts->ip_address_valid || !opts->port_valid || !opts->reg_address_valid || !opts->count_valid) {
			print_usage();
			return 1;
		}
	}

	return 0;
}
