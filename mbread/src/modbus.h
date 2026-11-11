

#ifndef __LUKAS_MODBUS_H
#define __LUKAS_MODBUS_H

typedef struct {
	uint8_t id;
	int socket_fd;

} mb_client_context_t;

int mb_client_context_init(mb_client_context_t *c, const char *ip_address, uint16_t port);
void mb_client_context_destroy(mb_client_context_t *c);

int mb_transaction_write_reg(mb_client_context_t *c, uint16_t address, uint16_t value);
int mb_transaction_read_seq_reg(mb_client_context_t *c, uint16_t address, uint16_t count);

#endif