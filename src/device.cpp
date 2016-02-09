#include "device.h"
#include "debug.h"
#include "serial.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace radiator {

/**
 * Constructor.
 *
 * @param devicename The filename of the serial device.
 */
Device::Device(std::string devicename)
: serialPort(devicename) {}

/**
 * Destructor.
 */
Device::~Device() {}

/**
 * Calculates the checksum of a device message.
 *
 * @param command The command structure whose checksum to calculate.
 * @return The 16 bit (unsigned) checksum.
 */
uint16_t Device::checksum(const uint8_t *command)
{
   uint16_t crc = 0;
   for (int i = 0; i < 3 + command[2]; ++i) {
      crc += command[i];
   }
   return crc;
}

/**
 * Verify the checksum of a device message.
 *
 * @param command The message whose checksum should be verified.
 * @return true if the checksum was OK, false if the checksum did not match.
 */
bool Device::checksum_verify(const uint8_t *command)
{
   uint16_t expected = this->checksum(command);
   uint16_t actual = ((uint16_t) command[3 + command[2]]) << 8 | command[3 + command[2] + 1];

   return expected == actual;
}

/**
 * Builds and sends a command to the serial line.
 *
 * @param cmd The two-byte command.
 * @param data The contents of the command.
 * @param len the number of bytes the data pointer holds.
 * @return The number of bytes written (len + 3) or -1 on error.
 */
int Device::send_cmd(uint8_t cmd[2], const uint8_t *data, uint8_t len)
{
   uint8_t sendBuffer[255 + 2 + 1 +2];

   sendBuffer[0] = cmd[0];
   sendBuffer[1] = cmd[1];
   sendBuffer[2] = len;
   ::memcpy(sendBuffer + 3, data, len);

   uint16_t crc = this->checksum(sendBuffer);
   sendBuffer[len + 3] = (crc & 0xff00) >> 8;
   sendBuffer[len + 4] = crc & 0x00ff;

   dump_string("> ", sendBuffer, 2 + 1 + len + 2, LOG_trace);

   return serialPort.write(sendBuffer, 2 + 1 + len + 2);
}

/**
 * Reads one message from serial.
 *
 * @param buffer Where to place the message. Must be at least 260 uint8_t big.
 * @param timeout The maximum time in ms to wait between incomming data.
 * @return The length of the message or -1 on error.
 */
int Device::read_cmd(uint8_t *buffer, int timeout)
{
   int len = 0;
   int expectedlen = 5;   // 2 cmd, 1 len, 2 crc

   while (len < expectedlen) {
      int ret = serialPort.waitForInput(timeout);

      if (ret > 0) {
         int bytes = serialPort.read(buffer + len, expectedlen - len);
         if (bytes >= 0) {
            dump_string("< ", buffer+len, bytes, LOG_trace);

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

}

