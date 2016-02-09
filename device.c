#include "device.h"
#include "debug.h"
#include "serial.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int device_init(const char *devicename)
{
   return open_port(devicename);
}

void device_deinit(int fd)
{
   close(fd);
}

uint16_t device_checksum(const uint8_t *command)
{
   uint16_t crc = 0;
   for (int i = 0; i < 3 + command[2]; ++i) {
      crc += command[i];
   }
   return crc;
}

uint8_t device_checksum_verify(const uint8_t *command)
{
   uint16_t expected = device_checksum(command);
   uint16_t actual = ((uint16_t) command[3 + command[2]]) << 8 | command[3 + command[2] + 1];

   return expected == actual;
}

int device_send_cmd(int fd, uint8_t cmd[2], const uint8_t *data, uint8_t len)
{
   uint8_t sendBuffer[255 + 2 + 1 +2];

   sendBuffer[0] = cmd[0];
   sendBuffer[1] = cmd[1];
   sendBuffer[2] = len;
   memcpy(sendBuffer + 3, data, len);

   uint16_t crc = device_checksum(sendBuffer);
   sendBuffer[len + 3] = (crc & 0xff00) >> 8;
   sendBuffer[len + 4] = crc & 0x00ff;

   dump_string("> ", sendBuffer, 2 + 1 + len + 2);

   return write_port(fd, sendBuffer, 2 + 1 + len + 2);
}

int device_read_cmd(int fd, uint8_t *buffer, int timeout)
{
   int len = 0;
   int expectedlen = 5;   // 2 cmd, 1 len, 2 crc

   while (len < expectedlen) {
      int ret = wait_input(fd, timeout);

      if (ret > 0) {
         int bytes = read_port(fd, buffer + len, expectedlen - len);
         if (bytes >= 0) {
            dump_string("< ", buffer+len, bytes);

            len += bytes;

            if (len >= 3) {
               expectedlen = 5 + buffer[2];
            }
         } else {
            return -1;
         }
      } else {
         return ret;
      }
   }
   return len;
}

