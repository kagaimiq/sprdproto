#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include "sprd_io.h"


int loadAndExecFile(char *path, uint32_t addr) {
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
			rc = sprdIoDoSendData(addr, dptr, dlen);
			
			free(dptr);
		}

		fclose(fp);

		if (rc >= 0) {
			rc = sprdIoDoExecData();
		} else {
			printf("Failed to load and exec file [%s] at %08x -- %d!\n",
				path, addr, rc);
			rc = 0; //second stage always returns error on this stage!
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

	/* ========= First stage ========== */
	puts("---------- First stage ---------");

	if (sprdIoOpen(528, false, 10000)) {
		printf("failed to open spreadtrum device [%d:%s]\n", errno, strerror(errno));
		goto Exit;
	}

	if (sprdIoDoHandshake()) {
		puts("bootrom handshake failed!");
		goto ExitClose;
	}

	if (sprdIoDoConnect()) {
		puts("bootrom connect failed!");
		goto ExitClose;
	}

	if (loadAndExecFile(argv[2], strtoul(argv[1], NULL, 0))) {
		puts("bootrom load&exec failed!");
		goto ExitClose;
	}

	/* ========= Second Stage ========= */
	if (argc >= 5) {
		puts("---------- Second stage ---------");
		sprdIoClose();

		/* wait to device disconnect then connect again */
		usleep(1000000);

		if (sprdIoOpen(2112, true, 2000)) {
			printf("failed to open fdl device [%d:%s]\n", errno, strerror(errno));
			goto Exit;
		}

		if (sprdIoDoHandshake()) {
			puts("fdl handshake failed!");
			goto ExitClose;
		}

		if (sprdIoDoConnect()) {
			puts("fdl connect failed!");
			goto ExitClose;
		}

		/* this always fails (the ack is then sent from U-Boot i guess) */
		loadAndExecFile(argv[4], strtoul(argv[3], NULL, 0));
	}

	rc = 0;
ExitClose:
	sprdIoClose();
Exit:
	libusb_exit(NULL);
	return rc;
}
