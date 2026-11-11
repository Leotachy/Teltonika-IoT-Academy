
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

int connect_to_server(const char *ip_address, __uint16_t port)
{
	// Make a TCP connection to the modbus server
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}

	struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(port) };

	if (inet_pton(AF_INET, ip_address, &addr.sin_addr) != 1) {
		puts("Provided IP address is not valid");
		exit(1);
	}

	if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr))) {
		perror("connect");
		exit(1);
	}

	// Setup timeout
	struct timeval tv = { .tv_sec = 2 };
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv)) {
		perror("timeout");
	}

	return fd;
}

int send_to_server(int fd, const uint8_t *bytes, ssize_t size)
{
	ssize_t sent = 0;
	int exit_code = 0;

	while (sent < size) {
		ssize_t sent_now = send(fd, &bytes[sent], size - sent, 0);
		if (sent_now == 0) {
			exit_code = 1;
			printf("Sent less bytes than expected! Sent %zd/%zd\n", sent, size);

			break;
		}

		if (sent_now < 0) {
			perror("send");
			exit_code = 1;
			break;
		}

		sent += sent_now;
	}

	return exit_code;
}

int receive_from_server(int fd, uint8_t *bytes, ssize_t size)
{
	ssize_t read = 0;
	int exit_code = 0;

	while (read < size) {
		ssize_t read_now = recv(fd, &bytes[read], size - read, 0);

		if (read_now == 0) {
			exit_code = 1; // Didn't read enough bytes
			printf("Read less bytes than expected! Read %zd/%zd\n", read, size);

			break;
		}

		if (read_now < 0) {
			perror("recv");
			exit_code = 1;
			break;
		}

		read += read_now;
	}

	return exit_code;
}