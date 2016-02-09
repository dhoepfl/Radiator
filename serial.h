#ifndef __DH_SERIAL_H__
#define __DH_SERIAL_H__

#include <inttypes.h>
#include <stdlib.h>

int open_port(const char *filename);
int write_port(int fd, const uint8_t *data, size_t len);
int read_port(int fd, uint8_t *data, size_t maxlen);
int wait_input(int fd, int timeout_msec);

#endif
