
// Lukas Andrijauskas
// Teltonika IoT akademija
// Oct-21-2026

// Simple LCD screen driver using I2C protocol

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/device/class.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/property.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include "i2c-lcd.h"
#include "simple_string.h"
#include "util.h"

// My custom implementation of a dynamic string..
static simple_string_t string;

static dev_t dev_num;
static struct cdev char_dev;
static struct class *cl;
static struct mutex lock;

static struct delayed_work scroll_page_work;
static u32 scroll_next_read;

// Time it takes to display a full page of characters
#define SCROLL_MAX_PAGE_TIME 5
#define SCROLL_MIN_PAGE_TIME 3

#define SCROLL_MAX_PAGES 3
#define SCROLL_MAX_NUM_CHARS_TOTAL (SCROLL_MAX_PAGES * LCD_SCREEN_MAX_CHARS)

static int setup_character_device(void);
static void clear_character_device(void);

static int mylcd_open(struct inode *inode, struct file *f) { return 0; }

static int mylcd_release(struct inode *inode, struct file *f) { return 0; }

static ssize_t mylcd_read(struct file *f, char __user *u, size_t l, loff_t *o) {
  if (*o >= string.length)
    return 0;

  u32 last_element_idx = *o + l;

  if (last_element_idx >= string.length)
    last_element_idx = string.length - 1;

  u32 len = last_element_idx - *o + 1;
  u32 not_copied = copy_to_user(u, &string.buffer[*o], len);
  u32 copied = len - not_copied;

  if (not_copied) {
    pr_warn(MYLCD_FMT("Copied only %u bytes to user buffer. Couldn't copy %u "
                      "bytes to user buffer"),
            copied, not_copied);
  }
  *o += copied;
  return copied;
}

static ssize_t mylcd_write(struct file *f, const char __user *u, size_t l,
                           loff_t *o) {
  u32 new_str_len = *o + l;

  if (new_str_len > SCROLL_MAX_NUM_CHARS_TOTAL)
    l -= new_str_len - SCROLL_MAX_NUM_CHARS_TOTAL;

  int written_bytes = 0;
  mutex_lock(&lock);
  { // Critical section
    written_bytes = simple_string_write_raw_bytes(&string, u, l, *o);
    scroll_next_read = 0; // Reset scrolling
    lcd_send_string(string.buffer);
  }
  mutex_unlock(&lock);

  *o += written_bytes;

  return written_bytes;
}

static const struct file_operations file_ops = {.open = mylcd_open,
                                                .release = mylcd_release,
                                                .write = mylcd_write,
                                                .read = mylcd_read};

static void mylcd_scroll_page_work(struct work_struct *work) {

  u32 next_update_time = SCROLL_MIN_PAGE_TIME;

  if (string.length > 0) {

    u32 old_scroll_next_read = 0;
    mutex_lock(&lock);
    { // Critical section

      old_scroll_next_read = scroll_next_read;

      if (scroll_next_read >= string.length)
        scroll_next_read = 0;

      pr_info(MYLCD_FMT("Page scrolling. Sending from %u. Old scroll %u"),
              scroll_next_read, old_scroll_next_read);
      const char *send_from = &string.buffer[scroll_next_read];
      scroll_next_read += lcd_send_string(send_from);
    }
    mutex_unlock(&lock);

    // Some logic for 'dynamic' update time
    u32 chars_sent = scroll_next_read - old_scroll_next_read;

    if (chars_sent >= (LCD_SCREEN_MAX_CHARS / 2))
      next_update_time = SCROLL_MAX_PAGE_TIME;
  }

  schedule_delayed_work(&scroll_page_work, secs_to_jiffies(next_update_time));
}

static int mylcd_probe(struct i2c_client *client) {
  int status;

  status = setup_character_device();
  if (status)
    return status;

  // Make our buffer where we hold text data
  status = simple_string_init(&string);
  if (status)
    return status;

  lcd_init(client);
  lcd_clear();

  mutex_init(&lock);

  INIT_DELAYED_WORK(&scroll_page_work, mylcd_scroll_page_work);
  schedule_delayed_work(&scroll_page_work,
                        secs_to_jiffies(SCROLL_MIN_PAGE_TIME));

  pr_info(MYLCD_FMT("Probed device with address: 0x%x"), client->addr);
  return status;
}

static void mylcd_remove(struct i2c_client *client) {
  cancel_delayed_work_sync(&scroll_page_work);

  mutex_destroy(&lock);

  lcd_clear();
  lcd_deinit();

  simple_string_deinit(&string);

  clear_character_device();

  pr_info(MYLCD_FMT("Removed"));
}

static const struct of_device_id mylcd_of_match_table[] = {
    {.compatible = "my_lcd"}, {}};

MODULE_DEVICE_TABLE(of, mylcd_of_match_table);

static struct i2c_driver mylcd_i2c_driver = {
    .driver = {.name = "my_lcd_drv",
               .of_match_table = mylcd_of_match_table,
               .owner = THIS_MODULE},
    .probe = mylcd_probe,
    .remove = mylcd_remove,
};

module_i2c_driver(mylcd_i2c_driver);

MODULE_AUTHOR("Lukas Andrijauskas");
MODULE_DESCRIPTION("My LCD driver with I2C");
MODULE_LICENSE("GPL");

static int setup_character_device(void) {
  int status = 0;

  status = alloc_chrdev_region(&dev_num, 0, MINORMASK + 1, "my_lcd");
  if (status) {
    pr_err(MYLCD_FMT("Failed allocating character device region"));
    return status;
  }

  cdev_init(&char_dev, &file_ops);
  char_dev.owner = THIS_MODULE;

  status = cdev_add(&char_dev, dev_num, MINORMASK + 1);
  if (status) {
    pr_err(MYLCD_FMT("Failed adding character device"));
    goto free_chrdev_region;
  }

  pr_info(MYLCD_FMT("Probed with char device maj - %d min - %d"),
          MAJOR(dev_num), MINOR(dev_num));

  cl = class_create("my_lcd");
  if (cl == NULL) {
    pr_err(MYLCD_FMT("Failed creating class"));
    status = ENOMEM;
    goto delete_cdev;
  }

  if (device_create(cl, NULL, dev_num, NULL, "my_lcd_%d", 0) == NULL) {
    pr_err(MYLCD_FMT("Failed creating device file"));
    status = ENOMEM;
    goto delete_class;
  }

  return status;

delete_class:
  device_destroy(cl, dev_num);
  class_destroy(cl);
delete_cdev:
  cdev_del(&char_dev);
free_chrdev_region:
  unregister_chrdev_region(dev_num, MINORMASK + 1);
  return status;
}

static void clear_character_device(void) {
  device_destroy(cl, dev_num);
  class_destroy(cl);
  cdev_del(&char_dev);
  unregister_chrdev_region(dev_num, MINORMASK + 1);
}
