#ifndef __DH_DEBUG_H__
#define __DH_DEBUG_H__

#include <inttypes.h>

extern uint8_t debug_level;

void dump_string(const char *prefix, const uint8_t *data, int len);

#endif
