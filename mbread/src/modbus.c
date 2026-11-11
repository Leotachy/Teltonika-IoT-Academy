
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "modbus.h"

#define MB_PROTOCOL_ID (0)
#define MB_UNIT_ID (1)

#define MB_READ_SEQ_REGISTERS_FID (0x03)
#define MB_WRITE_REG_FID (0x06)

#define MB_EXCEPTION_FLAG (1 << 7)

typedef enum {
	MB_EX_NONE,
	MB_EX_ILLEGAL_FUNCTION,
	MB_EX_ILLEGAL_DATA_ADDRESS,
	MB_EX_ILLEGAL_DATA_VALUE,
	MB_EX_SERVER_DEVICE_FAILURE,
} mb_exception_code_t;

// size of 8 bytes
typedef struct {
	uint16_t transaction_id; // 0-1
	uint16_t protocol_id; // 2-3
	uint16_t length; // 4-5
	uint8_t unit_id; // 6
	uint8_t function_code; // 7
} mb_adu_t;

// size of 12 bytes
typedef struct {
	mb_adu_t adu;

	uint16_t start_address; // 8-9
	uint16_t count; // 10-11
} mb_read_reg_req_t;

typedef struct {
	mb_adu_t adu;

	uint8_t byte_count; // 8
	uint16_t *register_values; // 8+byte_count

} mb_read_reg_rsp_t;

// size of 12 bytes
typedef struct {
	mb_adu_t adu;

	uint16_t address; // 8-9
	uint16_t value; // 10-11
} mb_write_reg_req_t;

typedef mb_write_reg_req_t mb_write_reg_rsp_t;

// Endian conversion helper functions

static void mb_adu_2be(mb_adu_t *adu)
{
	adu->transaction_id = htobe16(adu->transaction_id);
	adu->protocol_id = htobe16(adu->protocol_id);
	adu->length = htobe16(adu->length);
}

static void mb_read_reg_req_2be(mb_read_reg_req_t *r)
{
	mb_adu_2be(&r->adu);

	r->start_address = htobe16(r->start_address);
	r->count = htobe16(r->count);
}

static void mb_write_reg_req_2be(mb_write_reg_req_t *r)
{
	mb_adu_2be(&r->adu);

	r->address = htobe16(r->address);
	r->value = htobe16(r->value);
}

static void mb_adu_2h(mb_adu_t *adu)
{
	adu->transaction_id = be16toh(adu->transaction_id);
	adu->protocol_id = be16toh(adu->protocol_id);
	adu->length = be16toh(adu->length);
}

static void mb_read_reg_rsp_2h(mb_read_reg_rsp_t *r)
{
	mb_adu_2h(&r->adu);

	for (size_t i = 0; i < r->byte_count / sizeof(*r->register_values); i++)
		r->register_values[i] = be16toh(r->register_values[i]);
}

static void mb_write_reg_rsp_2h(mb_write_reg_rsp_t *r)
{
	mb_adu_2h(&r->adu);

	r->address = be16toh(r->address);
	r->value = be16toh(r->value);
}

static void mb_read_reg_req_init(mb_read_reg_req_t *r, uint16_t id, uint16_t start_address, uint16_t count)
{
	r->adu.transaction_id = id;
	r->adu.protocol_id = MB_PROTOCOL_ID;
	r->adu.length =
		sizeof(r->adu.unit_id) + sizeof(r->adu.function_code) + sizeof(r->start_address) + sizeof(r->count);
	r->adu.unit_id = MB_UNIT_ID;
	r->adu.function_code = MB_READ_SEQ_REGISTERS_FID;

	r->start_address = start_address;
	r->count = count;
}

static void mb_read_reg_rsp_destroy(mb_read_reg_rsp_t *r)
{
	if (r->register_values) {
		free(r->register_values);
		r->register_values = NULL;
	}
}

static void mb_write_reg_req_init(mb_write_reg_req_t *r, uint16_t id, uint16_t address, uint16_t value)
{
	r->adu.transaction_id = id;
	r->adu.protocol_id = MB_PROTOCOL_ID;
	r->adu.length = sizeof(r->adu.unit_id) + sizeof(r->adu.function_code) + sizeof(r->address) + sizeof(r->value);
	r->adu.unit_id = MB_UNIT_ID;
	r->adu.function_code = MB_WRITE_REG_FID;

	r->address = address;
	r->value = value;
}

static int mb_read_reg_req_send(mb_read_reg_req_t *r, int fd)
{
	mb_read_reg_req_2be(r);

	const uint8_t *bytes = (const uint8_t *)r;
	ssize_t size = sizeof(*r);

	// Print the bytes to console for debugging
	printf("Request: ");
	for (ssize_t i = 0; i < size; i++)
		printf("%02X ", bytes[i]);
	printf("\n");

	int status = send_to_server(fd, bytes, size);
	if (status) {
		puts("mb: Failed sending read register request");
	}

	return status;
}

static int mb_write_reg_req_send(mb_write_reg_req_t *r, int fd)
{
	mb_write_reg_req_2be(r);

	const uint8_t *bytes = (const uint8_t *)r;
	ssize_t size = sizeof(*r);

	// Print the bytes to console for debugging
	printf("Request: ");
	for (ssize_t i = 0; i < size; i++)
		printf("%02X ", bytes[i]);
	printf("\n");

	int status = send_to_server(fd, bytes, size);
	if (status) {
		puts("mb: Failed sending write register request");
	}

	return status;
}

static void mb_print_exception_code(mb_exception_code_t ex)
{
	printf("Exception: ");
	switch (ex) {
	case MB_EX_ILLEGAL_FUNCTION:
		puts("MB_EX_ILLEGAL_FUNCTION");
		break;
	case MB_EX_ILLEGAL_DATA_ADDRESS:
		puts("MB_EX_ILLEGAL_DATA_ADDRESS");
		break;
	case MB_EX_ILLEGAL_DATA_VALUE:
		puts("MB_EX_ILLEGAL_DATA_VALUE");
		break;
	case MB_EX_SERVER_DEVICE_FAILURE:
		puts("MB_EX_SERVER_DEVICE_FAILURE");
		break;
	default:
		puts("UNKNOWN!!");
		break;
	}
}

// Also deals with exceptions
static int mb_adu_recv(mb_adu_t *data, int fd, uint16_t tid, uint8_t fid)
{
	size_t read_size = sizeof(*data);
	int status = receive_from_server(fd, (uint8_t *)data, read_size);

	if (status) {
		puts("Failed reading adu");
		return status;
	}

	mb_adu_t adu_validate = *data;
	mb_adu_2h(&adu_validate);

	if (adu_validate.protocol_id != MB_PROTOCOL_ID) {
		printf("Bad protocol ID! Expected %hu, received %hu\n", MB_PROTOCOL_ID, adu_validate.protocol_id);
		return 1;
	}

	if (adu_validate.transaction_id != tid) {
		printf("Transaction ID mismatch! Expected %hu, received %hu\n", tid, adu_validate.transaction_id);
		return 1;
	}

	// Length must at least be big enough to cover the header ADU
	size_t minimum_length = sizeof(adu_validate.unit_id) + sizeof(adu_validate.function_code);
	if (adu_validate.length < minimum_length) {
		printf("Bad length! Expected at least %zu, received %hu\n", minimum_length, adu_validate.length);
		return 1;
	}

	if (adu_validate.unit_id != MB_UNIT_ID) {
		printf("Bad unit ID! Expected %hu, received %hu\n", MB_UNIT_ID, adu_validate.unit_id);
		return 1;
	}

	// Is there an exception?
	if (data->function_code & MB_EXCEPTION_FLAG) {
		// There is. Something went wrong

		uint8_t code;
		status = receive_from_server(fd, (uint8_t *)&code, sizeof(code));
		if (status) {
			puts("Failed reading exception code");
			return status;
		}

		mb_print_exception_code((mb_exception_code_t)code);
		return 1;
	}

	if (adu_validate.function_code != fid) {
		printf("Bad function ID! Expected %hhu, received %hhu\n", fid, adu_validate.function_code);
		return 1;
	}

	return 0;
}

static int mb_read_reg_rsp_recv(mb_read_reg_rsp_t *r, int fd, int tid)
{
	memset(r, 0, sizeof(*r));

	int status = mb_adu_recv(&r->adu, fd, tid, MB_READ_SEQ_REGISTERS_FID);
	if (status)
		return status;

	size_t size = sizeof(r->byte_count);
	status = receive_from_server(fd, (uint8_t *)&r->byte_count, size);
	if (status) {
		puts("Failed reading byte count");
		return status;
	}

	size = r->byte_count;

	r->register_values = (uint16_t *)malloc(size);
	if (r->register_values == NULL) {
		puts("Malloc failure");
		return 1;
	}

	// Read the register values finally
	status = receive_from_server(fd, (uint8_t *)r->register_values, size);
	if (status) {
		puts("Failed reading register values");
		goto cleanup_and_exit;
	}

	const uint8_t *bytes = (const uint8_t *)r;
	printf("Response: ");
	for (size_t i = 0; i < sizeof(r->adu) + sizeof(r->byte_count); i++)
		printf("%02X ", bytes[i]);
	for (size_t i = 0; i < r->byte_count; i++)
		printf("%02X ", ((uint8_t *)r->register_values)[i]);
	printf("\n");

	mb_read_reg_rsp_2h(r);

	// Some validation here
	size_t actual_length =
		sizeof(r->adu.unit_id) + sizeof(r->adu.function_code) + sizeof(r->byte_count) + r->byte_count;
	size_t expected_length = r->adu.length;

	if (actual_length != expected_length) {
		printf("Validation failure! Length %zu. Expected %zu", actual_length, expected_length);
		goto cleanup_and_exit;
	}

	return status;

cleanup_and_exit:
	free(r->register_values);
	return status;
}

static int mb_write_reg_rsp_recv(mb_write_reg_rsp_t *r, int fd, int tid)
{
	memset(r, 0, sizeof(*r));

	int status = mb_adu_recv(&r->adu, fd, tid, MB_WRITE_REG_FID);
	if (status)
		return status;

	size_t size = sizeof(r->address) + sizeof(r->value);
	status = receive_from_server(fd, (uint8_t *)&r->address, size);
	if (status) {
		puts("Failed reading address and value");
		return status;
	}

	const uint8_t *bytes = (const uint8_t *)r;
	printf("Response: ");
	for (size_t i = 0; i < sizeof(mb_write_reg_rsp_t); i++)
		printf("%02X ", bytes[i]);
	printf("\n");

	mb_write_reg_rsp_2h(r);

	// Some validation here
	size_t actual_length =
		sizeof(r->adu.unit_id) + sizeof(r->adu.function_code) + sizeof(r->address) + sizeof(r->value);
	size_t expected_length = r->adu.length;

	if (actual_length != expected_length) {
		printf("Validation failure! Length %zu. Expected %zu", actual_length, expected_length);
		return 1;
	}

	return status;
}

int mb_client_context_init(mb_client_context_t *c, const char *ip_address, uint16_t port)
{
	int fd = connect_to_server(ip_address, port);
	if (fd < 0)
		return fd;

	c->socket_fd = fd;
	c->id = 1;

	return 0;
}

void mb_client_context_destroy(mb_client_context_t *c)
{
	close(c->socket_fd);
}

int mb_transaction_write_reg(mb_client_context_t *c, uint16_t address, uint16_t value)
{
	uint16_t id = c->id++;

	mb_write_reg_req_t write_request;
	mb_write_reg_req_init(&write_request, id, address, value);

	if (mb_write_reg_req_send(&write_request, c->socket_fd)) {
		puts("Failed sending modbus write register request");
		return 1;
	}

	// Now receive back
	mb_write_reg_rsp_t response;
	if (mb_write_reg_rsp_recv(&response, c->socket_fd, id)) {
		puts("Failed receiving from server");
		return 1;
	}

	if (address != response.address || value != response.value) {
		printf("Received address and value don't match!"
		       " Expected address %hu, received %hu. Expected value %hu, received %hu.\n",
		       address, response.address, value, response.value);
		return 1;
	}
	printf("Register %hu value %hu\n", response.address, response.value);
	return 0;
}
int mb_transaction_read_seq_reg(mb_client_context_t *c, uint16_t address, uint16_t count)
{
	uint16_t id = c->id++;

	// Make the modbus read request
	mb_read_reg_req_t request;
	mb_read_reg_req_init(&request, id, address, count);

	if (mb_read_reg_req_send(&request, c->socket_fd)) {
		puts("Failed sending modbus read register request");
		return 1;
	}

	// Now receive back
	mb_read_reg_rsp_t response;
	if (mb_read_reg_rsp_recv(&response, c->socket_fd, id)) {
		puts("Failed receiving from server");
		return 1;
	}

	// Print register values
	for (size_t i = 0; i < (response.byte_count / sizeof(*response.register_values)); i++)
		printf("Register %zu: %hu\n", address + i, response.register_values[i]);

	mb_read_reg_rsp_destroy(&response);
	return 0;
}