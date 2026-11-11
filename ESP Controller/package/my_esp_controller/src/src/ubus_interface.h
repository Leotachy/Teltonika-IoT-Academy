

#ifndef __UBUS_INTERFACE_H
#define __UBUS_INTERFACE_H

int init_ubus_client(void);
void destroy_ubus_client(void);
void ubus_process(void);

#endif