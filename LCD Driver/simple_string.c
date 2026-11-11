

#include "linux/printk.h"
#include "linux/uaccess.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "simple_string.h"
#include "util.h"

#define SSTR_DEF_LENGTH 32
#define SSTR_GROWTH 4

int simple_string_init(simple_string_t *ss)
{
	ss->buffer = kzalloc(sizeof(char) * SSTR_DEF_LENGTH, GFP_KERNEL);
	if (ss->buffer == NULL)
		return -ENOMEM;

	ss->length = 0;
	ss->capacity = SSTR_DEF_LENGTH;

	return 0;
}

int simple_string_write_raw_bytes(simple_string_t *ss, const char __user *buf,
				  u32 len, u32 offset)
{
	u32 last_element_idx = len + offset + 1;

	if (last_element_idx > ss->capacity) {
		u32 new_capacity = last_element_idx;

		void *p = krealloc(ss->buffer, new_capacity, GFP_KERNEL);

		if (p == NULL) {
			pr_err(MYLCD_FMT(
				"Failed reallocating string buffer!!"));
			return -ENOMEM;
		}
		ss->buffer = p;
		ss->capacity = new_capacity;
	}

	// Do we have a gap
	if (offset >= (ss->length + 1)) {
		// We have a gap so fill it with empty spaces
		u32 gap_size = offset - ss->length;

		memset(&ss->buffer[ss->length], ' ', gap_size);
	}

	u32 not_copied = copy_from_user(&ss->buffer[offset], buf, len);
	u32 copied = len - not_copied;

	if (not_copied) {
		pr_warn(MYLCD_FMT(
				"Copied only %u bytes to simple string. Couldn't copy %u bytes"),
			copied, not_copied);
	}

	ss->buffer[last_element_idx - 1] = 0;
	ss->length = strlen(ss->buffer);
	return copied;
}

void simple_string_clear(simple_string_t *ss)
{
	if (ss->buffer)
		ss->buffer[0] = 0;

	ss->length = 0;
}

void simple_string_deinit(simple_string_t *ss)
{
	kfree(ss->buffer);
	ss->buffer = NULL;
	ss->capacity = 0;
	ss->length = 0;
}