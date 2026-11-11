
#ifndef __BLINKY_LED_H
#define __BLINKY_LED_H

// helper parsing funcs
int gpio_blinky_default_state_get(struct fwnode_handle *fwnode);
u32 gpio_blinky_interval_get(struct fwnode_handle *fwnode);
const char *gpio_blinky_name_get(struct fwnode_handle *fwnode,
                                 struct device *dev);

#endif
