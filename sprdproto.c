#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include "sprd_io.h"


int load_and_exec_file(char *path, uint32_t addr) {
	int rc = -1;
	FILE *fp;

	if ((fp = fopen(path, "rb"))) {
		fseek(fp, 0, SEEK_END);
		int dlen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		uint8_t *dptr = malloc(dlen);
		if (dptr) {
			dlen = fread(dptr, 1, dlen, fp);

			printf("Upload %d bytes into memory addr %08x!\n", dlen, addr);
			rc = sprd_io_send_data(addr, dptr, dlen);
			
			free(dptr);
		}

		fclose(fp);

		if (rc >= 0) {
			rc = sprd_io_exec_data();
		} else {
			printf("Failed to load file [%s] at %08x -- %d!\n",
				path, addr, rc);
		}
	}

	return rc;
}


int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: %s <payload addr> <payload file> [<2nd payload addr> <2nd payload file>]\n",
			argv[0]);

		puts(
		"\n"
		"The first payload will be loaded using the BootROM protocol,\n"
		" If the second payload is specified, then it will be loaded using the\n"
		" FDL protocol, which is provided by the first payload.\n"
		);
		
		return 1;
	}

	libusb_init(NULL);

	int rc = 2;

	if (sprd_io_open(528)) {
		printf("Failed to open a Spreadtrum device (%d - %s)\n", errno, strerror(errno));
		goto Exit;
	}

	if (sprd_io_handshake()) {
		puts("BootROM handshake failed!");
		goto ExitClose;
	}

	if (sprd_io_connect()) {
		puts("BootROM connect failed!");
		goto ExitClose;
	}

	if (load_and_exec_file(argv[2], strtoul(argv[1], NULL, 0))) {
		puts("BootROM load&exec failed!");
		goto ExitClose;
	}

	if (argc >= 5) {
		sprd_io_close();

		puts("=============== FDL1 ================");

		/* wait for device reconnection */
		usleep(1000000);

		if (sprd_io_open(2112)) {
			printf("Failed to open FDL1 device (%d - %s)\n", errno, strerror(errno));
			goto Exit;
		}

		if (sprd_io_handshake()) {
			puts("FDL1 handshake failed!");
			goto ExitClose;
		}

		if (sprd_io_connect()) {
			puts("FDL1 connect failed!");
			goto ExitClose;
		}

		load_and_exec_file(argv[4], strtoul(argv[3], NULL, 0));
	}

	rc = 0;
ExitClose:
	sprd_io_close();
Exit:
	libusb_exit(NULL);
	return rc;
}
