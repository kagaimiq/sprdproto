#ifndef _SPRDIO_H
#define _SPRDIO_H

#include <stdint.h>
#include <stdbool.h>

int sprd_io_open(int maxPktSize, bool useSprdChksum, int timeout);
void sprd_io_close(void);

int sprd_io_send(const uint8_t *data, int len);
int sprd_io_recv(uint8_t *data, int len);

int sprd_io_send_packet(uint16_t cmd, const uint8_t *data, uint16_t len);
int sprd_io_recv_packet(uint16_t *cmd, uint8_t *data, uint16_t len);

int sprd_io_send_cmd(uint16_t cmd, uint16_t *resp, const void *sdata, uint16_t slen, void *rdata, uint16_t rlen);

int sprd_io_handshake(void);
int sprd_io_connect(void);
int sprd_io_send_data(uint32_t addr, const uint8_t *data, uint32_t len);
int sprd_io_exec_data(void);

#endif
