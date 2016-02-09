#ifndef __DH_DEBUG_H__
#define __DH_DEBUG_H__

#include <inttypes.h>
#include <ostream>
#include <iostream>
#include <sstream>

static std::ostream &null()
{
   static std::ostringstream m_null;
   m_null.str("");
   m_null.clear();
   return m_null;
}

#define LOG_trace (debug_level >= 5 ? std::cerr : null())
#define LOG_debug (debug_level >= 4 ? std::cerr : null())
#define LOG_info  (debug_level >= 3 ? std::cerr : null())
#define LOG_warn  (debug_level >= 2 ? std::cerr : null())
#define LOG_error (debug_level >= 1 ? std::cerr : null())
#define LOG_fatal (std::cerr)

extern uint8_t debug_level;

void dump_string(const char *prefix,
                 const uint8_t *data,
                 int len,
                 std::ostream &ostream = LOG_debug);

#endif
