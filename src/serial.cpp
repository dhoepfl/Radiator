#include "serial.h"

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

namespace radiator {

/*
 * Constructor.
 *
 * Opens and initializes the serial port.
 *
 * Note that this class does accept a non-tty (simple file) as device but it
 * will never write anything to a non-tty. (Debug mode, read from file)
 *
 * @param devicename The filename of the serial port device.
 * @return The file descriptor on success or -1 on error.
 */
SerialPort::SerialPort(std::string devicename)
{
   this->fd = open(devicename.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
   if (this->fd == -1) {
      ::perror("open_port: Unable to open serial port");
      throw "Unable to open serial port";
   } else {
      if (-1 == ::fcntl(this->fd, F_SETFL, FNDELAY)) {
         ::perror("open_port: Failed to set no-delay");
         throw "Unable to open serial port";
      }

      if (::isatty(this->fd)) {
         struct termios options;
         if (-1 == ::tcgetattr(this->fd, &options)) {
            ::perror("open_port: Failed to get port options");
            throw "Unable to open serial port";
         }

         if (-1 == ::cfsetspeed(&options, B9600)) {
            ::perror("open_port: Failed to set port speed");
            throw "Unable to open serial port";
         }

         options.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
         options.c_cflag |= CLOCAL | CREAD | CS8;

         options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

         options.c_iflag &= ~(INPCK | PARMRK);
         options.c_iflag &= ~(IXON | IXOFF | IXANY);
         options.c_iflag &= ~(INLCR | IGNCR | ICRNL );
         options.c_iflag |= IGNPAR | IGNBRK | BRKINT;

         options.c_oflag &= ~OPOST;

         if (-1 == ::tcsetattr(this->fd, TCSANOW, &options)) {
            ::perror("open_port: Failed to set port attributes");
            throw "Unable to open serial port";
         }

         if (-1 == ::tcflow(this->fd, TCOON)) {
            ::perror("open_port: Failed to restart communication");
            throw "Unable to open serial port";
         }

         if (-1 == ::tcflow(this->fd, TCION) || -1 == ::tcdrain(this->fd)) {
            ::perror("open_port: Failed to restart communication");
            throw "Unable to open serial port";
         }

         if (-1 == ::tcflush(this->fd, TCIOFLUSH)) {
            ::perror("open_port: Failed to flush port");
            throw "Unable to open serial port";
         }

         if (-1 == ::tcsendbreak(this->fd, 0)) {
            ::perror("open_port: Failed to send break");
            throw "Unable to open serial port";
         }
      }
   }
}

/**
 * Destructor.
 */
SerialPort::~SerialPort()
{
   if (this->fd >= 0) {
      ::close(this->fd);
   }
}

/**
 * Writes the given data to the serial device.
 *
 * Note: Write does nothing if the serial device is a non-tty (ordinary file).
 *
 * @param data The data to send.
 * @param len The number of bytes to send.
 * @return len on success or -1 on error.
 */
int SerialPort::write(const uint8_t *data, size_t len)
{
   if (::isatty(this->fd)) {
      const uint8_t *pos = data;
      size_t left = len;

      while (left > 0) {
         int written = ::write(this->fd, pos, left);
         if (written < 0) {
            return -1;
         }

         left -= written;
         pos += written;
      }
   }

   return len;
}

/**
 * Reads data from the serial port if available.
 *
 * @param buffer The buffer to write data to.
 * @param maxlen The maximum number of bytes to read.
 * @return The number of bytes read or -1 on error.
 */
int SerialPort::read(uint8_t *data, size_t maxlen)
{
   int bytes;

   ::ioctl(this->fd, FIONREAD, &bytes);
   if (bytes > 0) {
      bytes = ::read(this->fd, data, maxlen);
   }

   return bytes;
}

/**
 * Wait for input being available.
 *
 * @param timeout_msec The maximum number of milliseconds to wait.
 * @return -1 on error, 0 if timeout occured. 1 if data available.
 */
int SerialPort::waitForInput(int timeout_msec)
{
   fd_set         input;
   struct timeval timeout = {0};

   timeout.tv_sec  = timeout_msec/1000;
   timeout.tv_usec = (timeout_msec - (timeout_msec/1000)*1000)*1000;

   FD_ZERO(&input);
   FD_SET(this->fd, &input);

   return ::select(this->fd+1, &input, NULL, NULL, &timeout);
}

}
