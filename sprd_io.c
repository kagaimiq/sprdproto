/*
 * This is pure mess.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "sprd_io.h"
#include "sprd_usbdev.h"

int max_data_sz, checksum_type;

/*----------------------------------------------------------------*/

static uint16_t calc_crc16(void *data, int len) {
	uint16_t crc = 0;

	while (len--) {
		crc ^= *(uint8_t*)(data++) << 8;

		for (int i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc >> 15) ? 0x1021 : 0);
	}

	return crc;
}

static uint16_t calc_sprdcheck(void *data, int len) {
	uint32_t ctr = 0;

	while (len > 0) {
		if (len >= 2) {
			ctr += *(uint16_t*)data;
			data += 2; len -= 2;
		} else {
			ctr += *(uint8_t*)data;
			data += 1; len -= 1;
		}
	}

	ctr = (ctr >> 16) + (ctr & 0xffff);
	ctr = ~(ctr + (ctr >> 16)) & 0xffff;
	return (ctr >> 8) | (ctr << 8);
}

static uint16_t calc_check(void *data, int len) {
	switch (checksum_type) {
	case 1:
		return calc_crc16(data, len);
	case 2:
		return calc_sprdcheck(data, len);
	default:
		return 0xd00b;
	}
}

static int check_data(void *data, int len, uint16_t check) {
	uint16_t calc;

	if (!checksum_type) {
		calc = calc_crc16(data, len);
		if (calc == check) {
			checksum_type = 1;
			goto Exit;
		}

		calc = calc_sprdcheck(data, len);
		if (calc == check) {
			checksum_type = 2;
			goto Exit;
		}
	} else {
		calc = calc_check(data, len);
	}

Exit:
	if (calc != check)
		printf("*** Checksum mismatch: (calc)%04x != (check)%04x ***\n", calc, check);

	return calc == check ? 0 : -1;
}

/*----------------------------------------------------------------*/

int sprd_io_open(int maxpktsz) {
	max_data_sz = maxpktsz;
	checksum_type = 0;

	return sprd_usbdev_open();
}

void sprd_io_close(void) {
	sprd_usbdev_close();
}

int sprd_io_send(void *ptr, int len) {
	return sprd_usbdev_write(ptr, len);
}

int sprd_io_recv(void *ptr, int len) {
	return sprd_usbdev_read(ptr, len);
}

/*----------------------------------------------------------------*/

/*
 * That's HDLC or PPP(oS), or whatever you may call it.
 * I initially though it was based off PPP, but in fact, PPP is based on HDLC afaik.
 */

int sprd_io_send_packet(uint16_t cmd, void *data, uint16_t len) {
	if (!data) len = 0;

	uint8_t *pktbody = malloc(2+2+len+2);	/* cmd+len+data+check */
	if (pktbody) {
		int rc = -1;

		/* Prepare command packet */
		int datalen = 0;
		/* Command */
		pktbody[datalen++] = cmd >> 8;
		pktbody[datalen++] = cmd >> 0;
		/* Data length */
		pktbody[datalen++] = len >> 8;
		pktbody[datalen++] = len >> 0;
		/* Data */
		memcpy(&pktbody[datalen], data, len);
		datalen += len;
		/* Checksum */
		uint16_t check = calc_check(pktbody, datalen);
		pktbody[datalen++] = check >> 8;
		pktbody[datalen++] = check >> 0;

		/* Calculate the real packet length */
		int pktlen = datalen;
		for (int i = 0; i < datalen; i++) {
			/* Account for an extra escape byte */
			if (pktbody[i] == 0x7d || pktbody[i] == 0x7e)
				pktlen++;
		}

		/* Make a packet */
		uint8_t *packet = malloc(1+pktlen+1);
		if (packet) {
			int n = 0;

			/* start */
			packet[n++] = 0x7e;

			/* body */
			for (int i = 0; i < datalen; i++) {
				/* Escape the bytes that shouldn't appear as-is */
				if (pktbody[i] == 0x7d || pktbody[i] == 0x7e) {
					packet[n++] = 0x7d;
					packet[n++] = pktbody[i] ^ 0x20;
					continue;
				}

				packet[n++] = pktbody[i];
			}

			/* end */
			packet[n++] = 0x7e;

			/* Send the packet! */
			if (sprd_io_send(packet, n) < 0)
				goto ExitFreePacket;

			rc = len;
ExitFreePacket:
			free(packet);
		}

		free(pktbody);
		return rc;
	}

	return -1;
}

int sprd_io_recv_packet(uint16_t *resp, void *data, uint16_t len) {
	if (!data) len = 0;

	/* packet length - start and stop, body is twice as large to fit escaped bytes */
	int maxlen = 1 + (2 + 2 + max_data_sz + 2)*2 + 1;

	uint8_t *packet = malloc(maxlen);
	if (packet) {
		/* Receive a packet */
		int rc = sprd_io_recv(packet, maxlen);

		/* Doesn't even fit these two markers? */
		if (rc <= 2) {
			rc = -1;
			goto ExitFreePacket;
		}

		/* Find packet start */
		int n = 0;
		while (n < rc) {
			if (packet[n++] == 0x7e) break;
		}

		/* Find packet end */
		int pktlen = 0;
		while (pktlen < rc) {
			if (packet[n + pktlen++] == 0x7e) break;
		}

		rc = -1;

		uint8_t *pktbody = malloc(pktlen);
		if (pktbody) {
			int datalen = 0;

			for (int i = 0; i < pktlen; i++) {
				/* Skip the begin/end marks */
				if (packet[i] == 0x7e)
					continue;

				/* De-escape the byte if this is an escape code */
				if (packet[i] == 0x7d) {
					pktbody[datalen++] = packet[++i] ^ 0x20;
					continue;
				}

				pktbody[datalen++] = packet[i];
			}

			/* If it doesn't fit at least cmd+len+check, then simply discard it. */
			if (datalen < 2+2+2)
				goto ExitFreePacketBody;

			n = 0;

			/* Get response code */
			*resp = pktbody[n+0] << 8 | pktbody[n+1];
			n += 2;

			/* Get data length */
			uint16_t dlen = pktbody[n+0] << 8 | pktbody[n+1];
			n += 2;

			/* Adjust the length (to the passed buffer size) */
			len = len < dlen ? len : dlen;

			/* Get the data */
			memcpy(data, &pktbody[n], len);
			n += dlen;

			/* Get the checksum and check it */
			uint16_t pktcheck = pktbody[n+0] << 8 | pktbody[n+1];

			if (check_data(pktbody, datalen - 2, pktcheck))
				goto ExitFreePacketBody;

			rc = len;

ExitFreePacketBody:
			free(pktbody);
		}

ExitFreePacket:
		free(packet);
		return rc;
	}

	return -1;
}

/*----------------------------------------------------------------*/

int sprd_io_send_cmd(uint16_t cmd, void *sdata, uint16_t slen, void *rdata, uint16_t rlen) {
	uint16_t resp;
	int rc;

	if ((rc = sprd_io_send_packet(cmd, sdata, slen)) < slen) {
		printf("[SendCMD] Failed to send command: %04x, [%p %d]! (%d)\n",
			cmd, sdata, slen, rc);
		return -1;
	}

	if ((rc = sprd_io_recv_packet(&resp, rdata, rlen)) < rlen) {
		printf("[SendCMD] Failed to receive response, [%p %d]! (%d)\n",
			rdata, rlen, rc);
		return -1;
	}

	return resp;
}

int sprd_io_handshake(void) {
	uint8_t tmp[64];
	uint16_t resp;
	int rc;

	if (sprd_io_send("\x7e", 1) < 1)
		return -1;

	if ((rc = sprd_io_recv_packet(&resp, tmp, sizeof tmp)) < 0)
		return -1;

	if (resp != BSL_REP_VER)
		return -resp;

	printf("*** Version: [%.*s] ***\n", rc, tmp);

	return 0;
}

int sprd_io_connect(void) {
	int rc;

	if ((rc = sprd_io_send_cmd(BSL_CMD_CONNECT, NULL, 0, NULL, 0)) < 0) {
		puts("[Connect] failed to send CMD_CONNECT!");
		return -1;
	}

	if (rc != BSL_REP_ACK) {
		printf("[Connect] CMD_CONNECT failed: %04x\n", rc);
		return -rc;
	}

	return 0;
}

int sprd_io_send_data(uint32_t addr, void *data, uint32_t len) {
	uint8_t tmp[8] = {addr>>24,addr>>16,addr>>8,addr, len>>24,len>>16,len>>8,len};
	int rc;

	if ((rc = sprd_io_send_cmd(BSL_CMD_START_DATA, tmp, sizeof tmp, NULL, 0)) < 0) {
		puts("[Send Data] failed to send CMD_START_DATA!");
		return -1;
	}

	if (rc != BSL_REP_ACK) {
		printf("[Send Data] CMD_START_DATA failed: %04x\n", rc);
		return -rc;
	}

	puts("[????????] ? (?)");

	for (uint32_t off = 0; off < len;) {
		int psize = len - off;
		if (psize > max_data_sz) psize = max_data_sz;

		printf("\e[1A[%08x] %d/%d (%d)\n",
			addr + off, off, len, psize);

		if ((rc = sprd_io_send_cmd(BSL_CMD_MIDST_DATA, data + off, psize, NULL, 0)) < 0) {
			puts("[Send Data] failed to send CMD_MID_DATA!");
			return -1;
		}

		if (rc != BSL_REP_ACK) {
			printf("[Send Data] CMD_MID_DATA failed: %04x\n", rc);
			return -rc;
		}

		off += psize;
	}

	if ((rc = sprd_io_send_cmd(BSL_CMD_END_DATA, NULL, 0, NULL, 0)) < 0) {
		puts("[Send Data] failed to send CMD_END_DATA!");
		return -1;
	}

	if (rc != BSL_REP_ACK) {
		printf("[Send Data] CMD_END_DATA failed: %04x\n", rc);
		return -rc;
	}

	return 0;
}

int sprd_io_exec_data(void) {
	int rc;

	if ((rc = sprd_io_send_cmd(BSL_CMD_EXEC_DATA, NULL, 0, NULL, 0)) < 0) {
		puts("[Exec Data] failed to send CMD_EXEC_DATA!");
		return -1;
	}

	if (rc != BSL_REP_ACK) {
		printf("[Exec Data] CMD_EXEC_DATA failed: %04x\n", rc);
		return -rc;
	}

	return 0;
}

