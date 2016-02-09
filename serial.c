#include "serial.h"

#define __USE_BSD
#define __USE_MISC

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

/*
 * 'open_port()' - Open serial port.
 *
 * Returns the file descriptor on success or -1 on error.
 */
int open_port(const char *filename)
{
   struct termios options;
   int fd; /* File descriptor for the port */

   fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
   if (fd == -1) {
      perror("open_port: Unable to open serial port - ");
   } else {
      fcntl(fd, F_SETFL, FNDELAY);

      tcgetattr(fd, &options);

      cfsetspeed(&options, B9600);
      options.c_cflag &= ~(CSIZE | CSTOPB | PARENB | CRTSCTS);
      options.c_cflag |= CLOCAL | CREAD | CS8;

      options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

      options.c_iflag &= ~(INPCK | PARMRK);
      options.c_iflag &= ~(IXON | IXOFF | IXANY);
      options.c_iflag &= ~(INLCR | IGNCR | ICRNL );
      options.c_iflag |= IGNPAR | IGNBRK;

      options.c_oflag &= ~OPOST;

      tcsetattr(fd, TCSANOW, &options);
   }

   return fd;
}

int write_port(int fd, const uint8_t *data, size_t len)
{
   const uint8_t *pos = data;
   size_t left = len;

   while (left > 0) {
      int written = write(fd, pos, left);
      if (written < 0) {
         return -1;
      }

      left -= written;
      pos += written;
   }

   return len;
}

int read_port(int fd, uint8_t *data, size_t maxlen)
{
   int bytes;

   ioctl(fd, FIONREAD, &bytes);
   if (bytes > 0) {
      bytes = read(fd, data, maxlen);
   }

   return bytes;
}

int wait_input(int fd, int timeout_msec)
{
   fd_set         input;
   struct timeval timeout = {0};

   timeout.tv_sec  = timeout_msec/1000;
   timeout.tv_usec = (timeout_msec - (timeout_msec/1000)*1000)*1000;

   FD_ZERO(&input);
   FD_SET(fd, &input);

   return select(fd+1, &input, NULL, NULL, &timeout);
}

