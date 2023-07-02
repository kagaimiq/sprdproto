#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include "sprd_usbdev.h"


#define SPRD_DEV_VID			0x1782
#define SPRD_DEV_PID			0x4D00


libusb_device_handle *devh;
int timeout = 1000;

uint8_t ep_in, ep_out;
int max_ep_sz;

/*-----------------------------------------------------------------------*/

static int parse_config(void) {
	libusb_device *dev = libusb_get_device(devh);
	struct libusb_config_descriptor *cfgdesc;
	const struct libusb_interface_descriptor *ifdesc;
	const struct libusb_endpoint_descriptor *epdesc;
	int config, rc = -1;

	if (libusb_get_configuration(devh, &config) < 0) {
		puts("can't get current configuration");
		return -1;
	}

	if (libusb_get_config_descriptor(dev, config - 1, &cfgdesc) < 0) {
		puts("can't get config descriptor");
		return -1;
	}

	/* Assume interface 0, altsetting 0 */
	ifdesc = &cfgdesc->interface[0].altsetting[0];

	/* Find the IN and OUT endpoints, alongside with their size */
	for (int i = 0; i < ifdesc->bNumEndpoints; i++) {
		epdesc = &ifdesc->endpoint[i];

		/* We need bulk endpoints */
		if (epdesc->bmAttributes != 0x02)
			continue;

		if (epdesc->bEndpointAddress & 0x80) {
			if (ep_in) continue;

			ep_in = epdesc->bEndpointAddress;
		} else {
			if (ep_out) continue;

			ep_out = epdesc->bEndpointAddress;
			max_ep_sz = epdesc->wMaxPacketSize;
		}
	}

	if (!ep_in || !ep_out) {
		puts("endpoints not found");
		goto Failed;
	}

	rc = 0;
Failed:
	libusb_free_config_descriptor(cfgdesc);
	return rc;
}

/*-----------------------------------------------------------------------*/

int sprd_usbdev_open(void) {
	printf("Waiting for a device.");

	while (!devh) {
		devh = libusb_open_device_with_vid_pid(NULL, SPRD_DEV_VID, SPRD_DEV_PID);
		putchar('.'); fflush(stdout);
		usleep(500000);
	}

	if (parse_config() < 0) {
		puts("failed: can't parse config descriptors");
		goto Failed;
	}

	if (libusb_claim_interface(devh, 0) < 0) {
		puts("failed: can't claim interface");
		goto Failed;
	}

	/*
	 * This is neccessary in order to "activate" the port.
	 */
	if (libusb_control_transfer(devh, 0x21, 34, 0x601, 0, NULL, 0, timeout) < 0) {
		puts("failed: can't send a control request");
		goto Failed2;
	}

	puts("ok!");
	return 0;

Failed2:
	libusb_release_interface(devh, 0);
Failed:
	libusb_close(devh);
	devh = NULL;
	return -1;
}

void sprd_usbdev_close(void) {
	if (!devh) return;

	libusb_release_interface(devh, 0);
	libusb_close(devh);
	devh = NULL;
}

int sprd_usbdev_write(void *data, int len) {
	if (!devh) return -1;

	/*
	 * Workaround for some issue caused by the invalid bulk endpoint size
	 * of 64 bytes, which worked before but now it doesn't.
	 */
	for (int i = 0; i < len; ) {
		int n = len - i;
		if (n > max_ep_sz) n = max_ep_sz;

		if (libusb_bulk_transfer(devh, ep_out, data, n, NULL, timeout) < 0)
			return -1;

		data += n;
		i += n;
	}

	return len;
}

int sprd_usbdev_read(void *data, int len) {
	if (!devh) return -1;

	int rxn;

	if (libusb_bulk_transfer(devh, ep_in, data, len, &rxn, timeout) < 0)
		return -1;

	return rxn;
}
