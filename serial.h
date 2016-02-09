#ifndef __DH_SERIAL_H__
#define __DH_SERIAL_H__

#include <inttypes.h>
#include <stdlib.h>
#include <string>

namespace radiator {

class SerialPort
{
public:
   SerialPort(std::string devicename);
   virtual ~SerialPort();

   int write(const uint8_t *data, size_t len);
   int read(uint8_t *data, size_t maxlen);

   int waitForInput(int timeout_msec);

protected:
   int fd;
};

}

#endif
