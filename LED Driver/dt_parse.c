

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/gpio.h>
#include <linux/gpio/consumer.h>

#include <linux/property.h>

#include "dt_parse.h"
// helper parsing funcs

int gpio_blinky_default_state_get(struct fwnode_handle *fwnode) {
  const char *state = NULL;
  if (!fwnode_property_read_string(fwnode, "default-state", &state)) {
    if (!strcmp(state, "on"))
      return 1;
  }

  return 0;
}

u32 gpio_blinky_interval_get(struct fwnode_handle *fwnode) {
  u32 interval = 0;
  fwnode_property_read_u32(fwnode, "interval", &interval);
  return interval;
}

const char *gpio_blinky_name_get(struct fwnode_handle *fwnode,
                                 struct device *dev) {
  const char *read_string = NULL;
  fwnode_property_read_string(fwnode, "label", &read_string);

  if (read_string)
    return devm_kstrdup_const(dev, read_string, GFP_KERNEL);
  return NULL;
}
