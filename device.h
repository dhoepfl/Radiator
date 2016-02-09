#ifndef __DH_DEVICE_H__
#define __DH_DEVICE_H__

#include <inttypes.h>

int device_init(const char *devicename);
void device_deinit(int fd);

uint16_t device_checksum(const uint8_t *command);
uint8_t device_checksum_verify(const uint8_t *command);

int device_send_cmd(int fd, uint8_t cmd[2], const uint8_t *data, uint8_t len);
int device_read_cmd(int fd, uint8_t *buffer, int timeout);

#endif
