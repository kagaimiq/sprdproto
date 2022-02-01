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
	int rc = 0;

	puts("spreadtrum io tool ...");
	printf("compiled %s - %s\n", __DATE__, __TIME__);
	
	if (argc < 3) {
		printf("Usage: %s <payload addr> <payload file> [<2nd payload addr> <2nd payload file>]\n",
			argv[0]);
		
		puts(
		"\n"
		"The first payload will be loaded using the BootROM protocol,\n"
		" If the second payload is specified, then it will be loaded using the\n"
		" BootBLK protocol, which is provided by the first payload.\n"
		);
		
		return 1;
	}

	libusb_init(NULL);
	
	/* ========= First stage ========== */
	puts("---------- First stage ---------");
	
	if (sprdIoOpen(528, false, 10000)) {
		printf("open spreadtrum device fail... %d - %s\n", errno, strerror(errno));
		rc = 2;
		goto Exit;
	}
	
	if ((rc = sprdIoDoHandshake())) {
		printf("bootrom handshake fail! [%d]\n", rc);
		goto Exit;
	}
	
	if ((rc = sprdIoDoConnect())) {
		printf("bootrom connect fail! [%d]\n", rc);
		goto Exit;
	}
	
	if ((rc = loadAndExecFile(argv[2], strtoul(argv[1], NULL, 0)))) {
		printf("bootrom load&exec fail! [%d]\n", rc);
		goto Exit2;
	}
	
	/* ========= Second Stage ========= */
	if (argc >= 5) {
		puts("---------- Second stage ---------");
		sprdIoClose();
		
		usleep(1000000);
		
		sprdIoOpen(2112, true, 2000);
		
		if ((rc = sprdIoDoHandshake())) {
			printf("bootblk handshake fail! [%d]\n", rc);
			goto Exit;
		}
		
		if ((rc = sprdIoDoConnect())) {
			printf("bootblk connect fail! [%d]\n", rc);
			goto Exit;
		}
		
		if ((rc = loadAndExecFile(argv[4], strtoul(argv[3], NULL, 0)))) {
			printf("bootblk load&exec fail! [%d]\n", rc);
			goto Exit2;
		}
	}

Exit2:
	sprdIoClose();
	
Exit:
	libusb_exit(NULL);
	return rc;
}
