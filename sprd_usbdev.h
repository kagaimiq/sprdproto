#ifndef _SPRD_USBDEV_H
#define _SPRD_USBDEV_H

#include <stdint.h>

int sprd_usbdev_open(void);
void sprd_usbdev_close(void);

int sprd_usbdev_write(void *data, int len);
int sprd_usbdev_read(void *data, int len);

#endif
