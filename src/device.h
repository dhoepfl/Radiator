#ifndef __DH_DEVICE_H__
#define __DH_DEVICE_H__

#include <inttypes.h>

#include "serial.h"

namespace radiator {

class Device
{
private:
   SerialPort  serialPort;

public:
   Device(std::string devicename);
   ~Device();

   uint16_t checksum(const uint8_t *command);
   bool checksum_verify(const uint8_t *command);

   int send_cmd(uint8_t cmd[2], const uint8_t *data, uint8_t len);
   int read_cmd(uint8_t *buffer, int timeout);
};

}

#endif
