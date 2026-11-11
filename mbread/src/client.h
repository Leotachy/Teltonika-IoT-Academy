

#ifndef __CLIENT_H
#define __CLIENT_H

int connect_to_server(const char *ip_address, uint16_t port);
int send_to_server(int fd, const uint8_t *bytes, ssize_t size);
int receive_from_server(int fd, uint8_t *bytes, ssize_t size);

#endif