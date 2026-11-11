
#ifndef __SIMPLE_STRING_H
#define __SIMPLE_STRING_H

#include <linux/init.h>
#include <linux/kernel.h>

// My very basic implementation of a dynamic char array

struct simple_string {
	char *buffer;

	// Doesn't count null term
	u32 length;
	u32 capacity;
};

typedef struct simple_string simple_string_t;

int simple_string_init(simple_string_t *ss);
int simple_string_write_raw_bytes(simple_string_t* ss, const char* __user buf, u32 len, u32 offset);
void simple_string_clear(simple_string_t *ss);
void simple_string_deinit(simple_string_t *ss);



#endif