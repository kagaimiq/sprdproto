#ifndef _SPRDIO_H
#define _SPRDIO_H

#include <stdint.h>
#include <stdbool.h>

enum {
	BSL_CMD_CONNECT			= 0x00,
	BSL_CMD_START_DATA,
	BSL_CMD_MIDST_DATA,
	BSL_CMD_END_DATA,
	BSL_CMD_EXEC_DATA
};

enum {
	BSL_REP_ACK			= 0x80,
	BSL_REP_VER,
	BSL_REP_INVALID_CMD,
	BSL_REP_UNKNOWN_CMD,
	BSL_REP_OPERATION_FAILED,
	BSL_REP_VERIFY_ERROR		= 0x8B,
	BSL_REP_NOT_VERIFY
};

/*--------------------------------------------------------------------------*/

/* Device I/O */
int sprd_io_open(int maxpktsz);
void sprd_io_close(void);
int sprd_io_send(void *ptr, int len);
int sprd_io_recv(void *ptr, int len);

/* Packet I/O */
int sprd_io_send_packet(uint16_t cmd, void *data, uint16_t len);
int sprd_io_recv_packet(uint16_t *resp, void *data, uint16_t len);

/* Command I/O */
int sprd_io_send_cmd(uint16_t cmd, void *sdata, uint16_t slen, void *rdata, uint16_t rlen);

/* Commands */
int sprd_io_handshake(void);
int sprd_io_connect(void);
int sprd_io_send_data(uint32_t addr, void *data, uint32_t len);
int sprd_io_exec_data(void);

#endif
