#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "sprd_io.h"

libusb_device_handle *sprdIoDev = NULL;
uint8_t sprdIoEpIn = 0x85, sprdIoEpOut = 0x06;
int sprdIoMaxDataSz = 528, sprdIoTimeout = 10000;
bool sprdIoUseSprdChksum = false;


uint16_t sprdChksumCalc(const void *data, int len) {
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

uint16_t crc16calc(const void *data, int len, uint16_t crc) {
	while (len--) {
		crc ^= *(uint8_t*)(data++) << 8;
		for (int i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc >> 15) ? 0x1021 : 0);
	}
	
	return crc;
}


int sprdIoOpen(int maxPktSize, bool useSprdChksum, int timeout) {
	printf("wait for sprd device.");
	while (sprdIoDev == NULL) {
		sprdIoDev = libusb_open_device_with_vid_pid(NULL, 0x1782, 0x4d00);
		putchar('.'); fflush(stdout);
		usleep(500000);
	}
	
	if (sprdIoDev) {
		sprdIoMaxDataSz = maxPktSize;
		sprdIoUseSprdChksum = useSprdChksum;
		sprdIoTimeout = timeout;
	
		//TODO: get endpoint stuff, etc
	
		if (libusb_control_transfer(sprdIoDev, 0x21, 34, 0x0601, 0x0000, NULL, 0, sprdIoTimeout) < 0) {
			libusb_close(sprdIoDev);
			sprdIoDev = NULL;
			printf("fail - cant send the control request\n");
			return -1;
		}
		
		printf("ok\n");
		return 0;
	}
	
	printf("fail - spurious error\n");
	return -1;
}

void sprdIoClose(void) {
	if (sprdIoDev) {
		//libusb_control_transfer(sprdIoDev, 0x21, 34, 0x0000, 0x0000, NULL, 0, sprdIoTimeout);
		libusb_close(sprdIoDev);
		sprdIoDev = NULL;
	}
}

int sprdIoSend(const uint8_t *data, int len) {
	if (sprdIoDev) {
		int trn;
		if (libusb_bulk_transfer(sprdIoDev, sprdIoEpOut, (uint8_t*)data, len, &trn, sprdIoTimeout) < 0) {
			return -1;
		}
		
		return trn;
	}
	return -1;
}

int sprdIoRecv(uint8_t *data, int len) {
	if (sprdIoDev) {
		int trn;
		if (libusb_bulk_transfer(sprdIoDev, sprdIoEpIn, data, len, &trn, sprdIoTimeout) < 0) {
			return -1;
		}
		
		return trn;
	}
	return -1;
}

int sprdIoSendPacket(uint16_t cmd, const uint8_t *data, uint16_t len) {
	if (!data) len = 0;
	
	uint8_t *packetc = malloc(2+2+len+2); //cmd+len+data+crc
	if (packetc) {
		int rc = -1;
		
		//------- Prepare command packet --------
		int packetcLen = 0;
		//cmd
		packetc[packetcLen++] = cmd >> 8;
		packetc[packetcLen++] = cmd >> 0;
		//len
		packetc[packetcLen++] = len >> 8;
		packetc[packetcLen++] = len >> 0;
		//data
		memcpy(&packetc[packetcLen], data, len);
		packetcLen += len;
		//crc
		uint16_t crc = 
				sprdIoUseSprdChksum?sprdChksumCalc(packetc, packetcLen)
				:crc16calc(packetc, packetcLen, 0x0000);
		packetc[packetcLen++] = crc >> 8;
		packetc[packetcLen++] = crc >> 0;
		
		//------ Calculate real packet length -------
		int packetLen = 0;
		for (int i = 0; i < packetcLen; i++) {
			packetLen++;
			if (packetc[i] == 0x7d) packetLen++;  //<-Escape symbol, escaped
			if (packetc[i] == 0x7e) packetLen++;  //<-Start/End marks, escaped
		}
		
		//------ Make real packet ------
		uint8_t *packet = malloc(1+packetLen+1); //start+contents+end
		if (packet) {
			int n = 0;
			//start
			packet[n++] = 0x7e;
			//contents
			for (int i = 0; i < packetcLen; i++) {
				//escape bytes that should not be in the packet as is
				if ((packetc[i] == 0x7d) || (packetc[i] == 0x7e)) {
					packet[n++] = 0x7d;
					packet[n++] = packetc[i] ^ 0x20;
					continue;
				}
				
				packet[n++] = packetc[i];
			}
			//end
			packet[n++] = 0x7e;

			rc = sprdIoSend(packet, n);
			if (rc < n) {
				rc = -1;
				goto Exit;
			}
			
			//since we check for full packet transmission, we'll just
			// put there the data length...
			rc = len;
Exit:
			free(packet);
		}

		free(packetc);
		return rc;
	}
	
	return -1;
}

int sprdIoRecvPacket(uint16_t *cmd, uint8_t *data, uint16_t len) {
	if (!data) len = 0;
	
	int maxPktLen = 1 + (2 + 2 + 65535 + 2)*2 + 1; //start+cmd+len+data+crc+end, twise as large to fit escaped bytes
	uint8_t *packet = malloc(maxPktLen);
	if (packet) {
		int rc = sprdIoRecv(packet, maxPktLen);
		if (rc <= 2) {
			rc = -1;
			goto Exit;
		}
		
		//------find packet start-------
		int n = 0;
		while (n < rc) {
			if (packet[n++] == 0x7e) break;
		}
		
		//-----find packet end------
		int pktSize = 0;
		while (pktSize < rc) {
			if (packet[n + pktSize++] == 0x7e) break;
		}

		//set rc to -1 for malloc failure
		rc = -1;

		uint8_t *packetc = malloc(pktSize);
		if (packetc) {
			int dataLenRecv = 0;
			//demangle it
			for (int i = 0; i < pktSize; i++) {
				//skip the start/end marks
				if (packet[i] == 0x7e)
					continue;
				
				//if this is the escape code, de-escape the byte after it
				if (packet[i] == 0x7d) {
					packetc[dataLenRecv++] = packet[++i] ^ 0x20;
					continue;
				}
				
				packetc[dataLenRecv++] = packet[i];
			}
			
			//if it can't hold cmd+len+crc in it, discard it
			if (dataLenRecv < 2+2+2) {
				rc = -1;
				goto Exit2;
			}
			
			//calculate the crc of packet contents excludng the crc itself
			uint16_t ccrc = 
				sprdIoUseSprdChksum?sprdChksumCalc(packetc, dataLenRecv-2)
				:crc16calc(packetc, dataLenRecv-2, 0x0000);
			
			n = 0;
			
			//gather the cmd
			*cmd = packetc[n++] << 8 | packetc[n++];
			
			//gather the data length
			uint16_t dlen = packetc[n++] << 8 | packetc[n++];
			//TODO: maybe adjust the dlen to the actual received data length??
			
			//adjust the length
			len = len < dlen ? len : dlen;
			
			//copy the data into the buffer
			memcpy(data, &packetc[n], len); n += dlen;
			
			//gather the crc and check if it's valid
			uint16_t crc = packetc[n++] << 8 | packetc[n++];
			if (crc != ccrc) {
				printf("!!! INVAID CRC recv-%04x != calc-%04x\n", crc, ccrc);
				rc = -1;
				goto Exit2;
			}
			
			//set the rc to the copied length
			rc = len;
			
Exit2:
			free(packetc);
		}

Exit:
		free(packet);
		return rc;
	}
	
	return -1;
}

int sprdIoDoSendCmd(uint16_t cmd, uint16_t *resp, const void *sdata, uint16_t slen, void *rdata, uint16_t rlen) {
	int rc;
	
	if ((rc = sprdIoSendPacket(cmd, sdata, slen)) < slen) {
		printf("[SendCMD] failed to send cmd %04x, [%p %d]! [%d]\n",
			cmd, sdata, slen, rc);
		if (rc >= 0) rc = -1;
		return rc;
	}
	
	if ((rc = sprdIoRecvPacket(resp, rdata, rlen)) < rlen) {
		printf("[SendCMD] failed to recv resp, [%p %d]! [%d]\n",
			rdata, rlen, rc);
		if (rc >= 0) rc = -1;
		return rc;
	}
	
	return 0;
}

int sprdIoDoHandshake(void) {
	uint8_t tmp[64] = {0x7e};
	uint16_t pktResp;
	int rc;
	
	if ((rc = sprdIoSend(tmp, 1)) < 1) {
		if (rc >= 0) rc = -1;
		return rc;
	}
	
	if ((rc = sprdIoRecvPacket(&pktResp, tmp, sizeof tmp)) <= 0) {
		if (rc >= 0) rc = -1;
		return rc;
	}
	
	printf("===> [%.*s]\n", rc, tmp);
	
	if (pktResp != 0x0081) {
		return -1;
	}

	return 0;
}

int sprdIoDoConnect(void) {
	uint16_t pktResp;
	int rc;
	
	//------- Send CMD_CONNECT --------
	if ((rc = sprdIoDoSendCmd(0x0000, &pktResp, NULL, 0, NULL, 0)) < 0) {
		printf("[Connect] failed to send CMD_CONNECT! [%d]\n", rc);
		return rc;
	}
	
	if (pktResp != 0x0080) {
		printf("[Connect] invalid response of CMD_CONNECT! %04x!\n", pktResp);
		return -1;
	}
	
	return 0;
}

int sprdIoDoSendData(uint32_t addr, const uint8_t *data, uint32_t len) {
	uint8_t tmp[8] = {addr>>24,addr>>16,addr>>8,addr, len>>24,len>>16,len>>8,len};
	uint16_t pktResp;
	int rc;
	
	//------- Send CMD_START_DATA --------
	if ((rc = sprdIoDoSendCmd(0x0001, &pktResp, tmp, 8, NULL, 0)) < 0) {
		printf("[Send Data] failed to send CMD_START_DATA! [%d]\n", rc);
		return rc;
	}
	
	if (pktResp != 0x0080) {
		printf("[Send Data] invalid response of CMD_START_DATA! %04x!\n", pktResp);
		return -1;
	}
	
	//------- transfer all data ---------
	puts("[????????] ? (?)");
	
	uint32_t doff = 0;
	for (;;) {
		int psize = len > sprdIoMaxDataSz ? sprdIoMaxDataSz : len;
		if (psize <= 0) break;
		
		printf("\e[1A[%08x] %d (%d)\n",
			addr+doff, doff, psize);
		
		//------- Send CMD_MID_DATA --------
		if ((rc = sprdIoDoSendCmd(0x0002, &pktResp, data+doff, psize, NULL, 0)) < 0) {
			printf("[Send Data] failed to send CMD_MID_DATA! [%d]\n", rc);
			return rc;
		}
		
		if (pktResp != 0x0080) {
			printf("[Send Data] invalid response of CMD_MID_DATA! %04x!\n", pktResp);
			return -1;
		}
		
		doff += psize;
		len -= psize;
	}

	//------- Send CMD_END_DATA --------
	if ((rc = sprdIoDoSendCmd(0x0003, &pktResp, NULL, 0, NULL, 0)) < 0) {
		printf("[Send Data] failed to send CMD_END_DATA! [%d]\n", rc);
		return rc;
	}
	
	if (pktResp != 0x0080) {
		printf("[Send Data] invalid response of CMD_END_DATA! %04x!\n", pktResp);
		return -1;
	}
	
	return 0;
}

int sprdIoDoExecData(void) {
	uint16_t pktResp;
	int rc;
	
	//------- Send CMD_EXEC_DATA --------
	if ((rc = sprdIoDoSendCmd(0x0004, &pktResp, NULL, 0, NULL, 0)) < 0) {
		printf("[Send Data] failed to send CMD_EXEC_DATA! [%d]\n", rc);
		return rc;
	}
	
	if (pktResp != 0x0080) {
		printf("[Send Data] invalid response of CMD_EXEC_DATA! %04x!\n", pktResp);
		return -1;
	}
	
	return 0;
}
