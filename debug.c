#include "debug.h"

#include <stdio.h>
#include <ctype.h>

uint8_t debug_level = 1;

static void dump_string_line(const uint8_t *data, int len)
{
   int i;

   for (i = 0; i < len; ++i) {
      fprintf(stdout, "%02x ", data[i]);
   }
   while (i % 16 != 0) {
      fprintf(stdout, ".. ");
      ++i;
   }

   fprintf(stdout, " |  ");
   for (i = 0; i < len; ++i) {
      if (isprint(data[i])) {
         fprintf(stdout, "%c", data[i]);
      } else {
         fprintf(stdout, ".");
      }
   }
   fprintf(stdout, "\n");
}

void dump_string(const char *prefix, const uint8_t *data, int len)
{
   if (debug_level) {
      for (int i = 0; i < len; i += 16) {
         fprintf(stdout, "%s", prefix);
         dump_string_line(data+i, len >= i+16 ? 16 : len-i);
      }
   }
}


