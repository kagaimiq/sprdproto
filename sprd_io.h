#ifndef _SPRDIO_H
#define _SPRDIO_H

#include <stdint.h>
#include <stdbool.h>

int sprdIoOpen(int maxPktSize, bool useSprdChksum, int timeout);
void sprdIoClose(void);

int sprdIoSend(const uint8_t *data, int len);
int sprdIoRecv(uint8_t *data, int len);

int sprdIoSendPacket(uint16_t cmd, const uint8_t *data, uint16_t len);
int sprdIoRecvPacket(uint16_t *cmd, uint8_t *data, uint16_t len);

int sprdIoDoSendCmd(uint16_t cmd, uint16_t *resp, const void *sdata, uint16_t slen, void *rdata, uint16_t rlen);

int sprdIoDoHandshake(void);
int sprdIoDoConnect(void);
int sprdIoDoSendData(uint32_t addr, const uint8_t *data, uint32_t len);
int sprdIoDoExecData(void);

#endif
