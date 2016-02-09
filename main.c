#include "debug.h"
#include "device.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

typedef enum
{
   ST_ERROR = -1,
   ST_STARTING = 0,
   ST_RA_SENT,
   ST_RB_SENT
} STATE;

typedef struct
{
   int      fd;
   STATE    state;
   char     *statusMessages[256][256];
   char     *errorMessages[256];
   char     *operationMode[256];
} INFORMATION;

static uint8_t handle_command(INFORMATION *dev, const uint8_t *command)
{
   if (command[0] == 'M') {
      switch(command[1]) {
         case '1':   // Messwerte
            if (dev->state == ST_RB_SENT) {
            }
            break;
         case '2':   // Uhrzeit / Datum
            if (command[2] != 7) {
               return ST_ERROR;
            }

            char *dow[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
            fprintf(stdout,
                    "Date/Time: %s, 20%c%c-%c%c-%c%c, %c%c:%c%c:%c%c\n",
                    dow[command[8]],
                    '0' + (command[9] >> 4), '0' + (command[9] & 0x0f),
                    '0' + (command[7] >> 4), '0' + (command[7] & 0x0f),
                    '0' + (command[6] >> 4), '0' + (command[6] & 0x0f),
                    '0' + (command[5] >> 4), '0' + (command[5] & 0x0f),
                    '0' + (command[4] >> 4), '0' + (command[4] & 0x0f),
                    '0' + (command[3] >> 4), '0' + (command[3] & 0x0f));

            if (dev->state == ST_RA_SENT) {
               if (device_send_cmd(dev->fd, (uint8_t *) "Rb", (uint8_t *) "\0\0\0", 3) < 0) {
                  perror("Failed to send Rb command");
                  return ST_ERROR;
               }

               return ST_RB_SENT;
            }
            break;
         case 'A':   // Texts of main display
            break;
         case 'B':   // Status messages
         {
            uint16_t page = (((uint16_t) command[3]) << 8) | command[4];
            uint16_t messageID = (((uint16_t) command[5]) << 8) | command[6];

            if (page > 255 || messageID > 255) {
               fprintf(stderr, "Status message (%d,%d) out of range, ignored\n", page, messageID);
            } else {
               free(dev->statusMessages[page][messageID]);
               dev->statusMessages[page][messageID] = malloc(command[2]-3);
               if (!dev->statusMessages[page][messageID]) {
                  fprintf(stderr, "Out of memory");
                  dev->state = ST_ERROR;
               } else {
                  memcpy(dev->statusMessages[page][messageID], command+7, command[2]-4);
                  dev->statusMessages[page][messageID][command[2]-4] = '\0';
               }
            }
            break;
         }
         case 'D':   // Texts of settings menu
            break;
         case 'E':   // Parameter format information
         {
            if (command[2] != 0x11) {
               dev->state = ST_ERROR;
            } else {
               uint16_t paramID = (((uint16_t) command[3]) << 8) | command[4];
               uint8_t unit = command[5];
               uint8_t comma = command[6];
               uint16_t factor = (((uint16_t) command[7]) << 8) | command[8];
               uint16_t min = (((uint16_t) command[9]) << 8) | command[10];
               uint16_t max = (((uint16_t) command[11]) << 8) | command[12];
               uint16_t std = (((uint16_t) command[13]) << 8) | command[14];
               uint16_t value = (((uint16_t) command[18]) << 8) | command[19];

               fprintf(stdout, "%04d: %d %d %d %d %d %d %d\n",
                       paramID,
                       (int) unit,
                       (int) comma,
                       factor,
                       min, max, std,
                       value);
            }
            break;
         }
         case 'F':   // Modes of operation
         {
            uint8_t modeID = command[3];

            free(dev->operationMode[modeID]);
            dev->operationMode[modeID] = malloc(command[2]);
            if (!dev->operationMode[modeID]) {
               fprintf(stderr, "Out of memory");
               dev->state = ST_ERROR;
            } else {
               memcpy(dev->operationMode[modeID], command+4, command[2]-1);
               dev->operationMode[modeID][command[2]-1] = '\0';
            }
            break;
         }
         case 'G':   // Weekly programs
            break;
         case 'T':   // Error texts
         {
            uint8_t errorID = command[3];

            free(dev->errorMessages[errorID]);
            dev->errorMessages[errorID] = malloc(command[2]);
            if (!dev->errorMessages[errorID]) {
               fprintf(stderr, "Out of memory");
               dev->state = ST_ERROR;
            } else {
               memcpy(dev->errorMessages[errorID], command+4, command[2]-1);
               dev->errorMessages[errorID][command[2]-1] = '\0';
            }
            break;
         }
         case 'U':   // Last errors
            break;
         case 'Z':   // End of block
            if (debug_level) {
               switch (command[3]) {
                  case 'B':
                     fprintf(stdout, "Status messages:\n");
                     for (int page = 0; page < 256; ++page) {
                        for (int i = 0; i < 256; ++i) {
                           if (dev->statusMessages[page][i]) {
                              fprintf(stdout, "%02x/%02x (%03d/%03d): %s\n",
                                      page, i,
                                      page, i,
                                      dev->statusMessages[page][i]);
                           }
                        }
                     }
                     break;
                  case 'F':
                     fprintf(stdout, "Operation modes: \n");
                     for (int i = 0; i < 256; ++i) {
                        if (dev->operationMode[i]) {
                           fprintf(stdout, "%02x (%03d): %s\n", i, i, dev->operationMode[i]);
                        }
                     }
                     break;
                  case 'T':
                     fprintf(stdout, "Errorcodes: \n");
                     for (int i = 0; i < 256; ++i) {
                        if (dev->errorMessages[i]) {
                           fprintf(stdout, "%02x (%03d): %s\n", i, i, dev->errorMessages[i]);
                        }
                     }
                     break;
               }
            }
            break;
         case 'C':   // meaning unknown
         case 'H':   // meaning unknown
         case 'I':   // meaning unknown
         case 'K':   // meaning unknown
         case 'L':   // meaning unknown
         case 'M':   // meaning unknown
         case 'S':   // meaning unknown
         case 'W':   // meaning unknown
            break;
         default:
            fprintf(stderr, "Unknown command %c%c\n", command[0], command[1]);
            break;
      }
   } else {
      fprintf(stderr, "Unknown command %c%c\n", command[0], command[1]);
   }

   return 1;
}

void main_loop(const char *filename, int timeout)
{
   INFORMATION p2 = {
      .fd = -1,
      .state = ST_STARTING,
      .statusMessages = {0},
      .errorMessages = {0},
      .operationMode = {0}
   };

   p2.fd = device_init(filename);
   if (p2.fd < 0) {
      return;
   }

   // device_send_cmd(p2.fd, (uint8_t *) "MT", (uint8_t *) "\05Fernsteller2\012fehlerhaft", 0x19);
   // device_send_cmd(p2.fd, (uint8_t *) "MZ", (uint8_t *) "T", 0x01);
   // device_send_cmd(p2.fd, (uint8_t *) "MB", (uint8_t *) "\0\2\0\2  Anheizen     ", 0x13);
   // device_send_cmd(p2.fd, (uint8_t *) "MZ", (uint8_t *) "B", 0x01);

   // Log in.
   p2.state = ST_RA_SENT;
   if (device_send_cmd(p2.fd, (uint8_t *) "Ra", (uint8_t *) "\0\xff\xf9", 3) < 0) {
      perror("Failed to send login command - ");
      return;
   }

   sleep(1);
   for(;;) {
      uint8_t buffer[2+1+255+2];
      int len = device_read_cmd(p2.fd, buffer, timeout);

      switch(len) {
         case -1:
            perror("Reading answer failed");
            goto shutdown;
         case 0:
            fprintf(stderr, "Timeout\n");
            goto shutdown;
         default:
         {
            if (device_checksum_verify(buffer)) {
               fprintf(stderr, "Got an cmd %c%c, %d bytes\n", buffer[0], buffer[1], buffer[2]);
               if (buffer[2] == 1 && buffer[3] == 1) {
                  // ACK.

                  fprintf(stderr, "Just an ACK, ignored.\n");
               } else {
                  // DATA, send ACK:
                  device_send_cmd(p2.fd, buffer, (uint8_t *) "\1", 1);

                  handle_command(&p2, buffer);

                  if (p2.state == ST_ERROR) {
                     fprintf(stderr, "Protocol error\n");
                     goto shutdown;
                  }
               }
            } else {
               fprintf(stderr, "Invalid response, checksum error\n");
               goto shutdown;
            }
         }
      }
   }
shutdown:
   for (int i = 0; i < 256; ++i) {
      for (int j = 0; j < 256; ++j) {
         free(p2.statusMessages[i][j]);
      }
      free(p2.errorMessages[i]);
      free(p2.operationMode[i]);
   }

   device_deinit(p2.fd);
}

int main(int argc, char *argv[])
{
   debug_level = 0;

   int timeout = 1500;

   for (int c = 0; (c = getopt(argc, argv, "+DT:")) != -1;) {
      switch (c) {
         case 'D':
            debug_level++;
            break;
         case 'T':
            timeout = atoi(optarg)*1000;
            if (timeout < 1000) {
               fprintf(stderr, "Min timeout 1s!\n");
               timeout = 1000;
            }

            break;
         case '?':
            if (optopt == 'c')
               fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
               fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            exit(1);
         case 'h':
            fprintf(stderr, "usage: %s [-D] [-T <timeout in s>] <serial port name>\n", argv[0]);
            exit(1);
      }
   }

   if (argc - optind != 1) {
      fprintf(stderr, "usage: %s [-D] [-T <timeout in s>] <serial port name>\n", argv[0]);
      exit(1);
   }

   char *devicename = argv[optind];
   if (debug_level) {
      fprintf(stderr, "Device name: %s\n", devicename);
   }

   for(;;) {
      fprintf(stderr, "Starting main loop\n");
      main_loop(devicename, timeout);

      fprintf(stderr, "Main loop ended, restarting in 10s\n");
      sleep(10);
   }
}
