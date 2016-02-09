#include "debug.h"

#include <cctype>
#include <iostream>
#include <iomanip>

uint8_t debug_level = 1;

static const int LINE_LEN = 24;

static void dump_string_line(const uint8_t *data, int len, std::ostream &ostream)
{
   int i;

   for (i = 0; i < len; ++i) {
      ostream << std::setfill('0') << std::setw(2) << std::hex << ((unsigned int) data[i]) << std::dec << " ";
   }
   while (i % LINE_LEN != 0) {
      ostream << ".. ";
      ++i;
   }

   ostream << " |  ";
   for (i = 0; i < len; ++i) {
      if (::isprint(data[i])) {
         ostream << ((char) data[i]);
      } else {
         ostream << ".";
      }
   }
   ostream << std::endl;
}

void dump_string(const char *prefix, const uint8_t *data, int len, std::ostream &ostream)
{
   for (int i = 0; i < len; i += LINE_LEN) {
      ostream << prefix;
      dump_string_line(data+i, len >= i+LINE_LEN ? LINE_LEN : len-i, ostream);
   }
}


