

#ifndef __UTIL_H
#define __UTIL_H

#define LU_PR_INFO(x, ...) printf(x "\n", ##__VA_ARGS__)
#define LU_PR_ERR(x, ...) fprintf(stderr, x "\n", ##__VA_ARGS__)

static inline int to_hex_string(char *buf, int buf_size, int value)
{
	if (buf_size <= 1) {
		LU_PR_ERR("to_hex_string: buffer too small");
		return 1;
	}

	snprintf(buf, buf_size - 1, "0x%x", value);
	return 0;
}

#endif