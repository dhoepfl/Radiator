#include "surveillance.h"
#include "debug.h"
#include "cp850_to_utf8.h"

#include <unistd.h>

#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <list>

namespace radiator {

/**
 * Constructor.
 *
 * @param devicename The filename of the serial device.
 * @param timeout The maximum duration in ms between data packets until the
 *                connection is considered stale.
 * @param handler The handler to inform about events.
 */
Surveillance::Surveillance(std::string devicename, int timeout, SurveillanceHandler &handler)
: handler(handler),
  fd(devicename),
  timeout(timeout),
  state(ST_STARTING) {}

/**
 * Destructor.
 */
Surveillance::~Surveillance() {}

/**
 * The main loop. Does one round of device protocol handling. Will return on error.
 */
void Surveillance::main_loop()
{
   // Log in.
   this->state = ST_RA_SENT;
   if (this->fd.send_cmd((uint8_t *) "Ra", (uint8_t *) "\0\xff\xf9", 3) < 0) {
      perror("Failed to send login command - ");
      return;
   }

   // Handle incomming packets.
   while (this->state != ST_ERROR) {
      uint8_t buffer[2+1+255+2];
      int len = this->fd.read_cmd(buffer, timeout);

      switch(len) {
         case -1:
            perror("Reading answer failed");
            this->state = ST_ERROR;
            break;
         case 0:
            LOG_error << "Timeout: device did not send any data within " << timeout << "ms." << std::endl;
            this->state = ST_ERROR;
            break;
         default:
         {
            if (this->fd.checksum_verify(buffer)) {
               if (debug_level) {
                  LOG_debug
                     << "Got an cmd `"
                     << (::isprint(buffer[0]) ? (char) buffer[0] : '.')
                     << (::isprint(buffer[1]) ? (char) buffer[1] : '.')
                     << std::hex
                     << "' (0x" << std::setfill('0') << std::setw(2) << (unsigned int) buffer[0]
                     << " 0x" << std::setfill('0') << std::setw(2) << (unsigned int) buffer[1]
                     << std::dec
                     << "), " << (int) buffer[2] << " bytes" << std::endl;
                  dump_string("+ ", buffer, buffer[2]+5, LOG_debug);
               }

               if (buffer[2] == 1 && buffer[3] == 1) {
                  // ACK.
                  LOG_trace << "Just an ACK, ignored." << std::endl;
               } else {
                  // DATA, send ACK:
                  this->fd.send_cmd(buffer, (uint8_t *) "\1", 1);

                  this->handle_command(buffer);
               }
            } else {
               LOG_error << "Invalid response, checksum error." << std::endl;
               this->state = ST_ERROR;
            }
         }
      }
   }

   LOG_error << "Protocol error" << std::endl;
}

/**
 * Handles one message.
 *
 * @param command The command read.
 */
void Surveillance::handle_command(const uint8_t *command)
{
   if (command[0] == 'M') {
      switch(command[1]) {
         case '1':   // Measurements
            this->parseMeasurement(command);
            break;
         case '2':   // Time / Date
            this->parseDateTime(command);
            break;
         case '3':   // Failure!
            this->parseFailure(command);
            break;
         case 'A':   // Texts of main display
            this->parseParameterNames(command);
            break;
         case 'B':   // Display texts
            this->parseDisplayTexts(command);
            break;
         case 'C':   // Dataformat of M1
            this->parseDataformats(command);
            break;
         case 'D':   // Texts of settings menu
            this->parseSettingsText(command);
            break;
         case 'E':   // Parameter format information
            this->parseFormats(command);
            break;
         case 'F':   // Modes of operation
            this->parseOperationMode(command);
            break;
         case 'G':   // Weekly programs
            this->parseWeeklyProgram(command);
            break;
         case 'I':   // Parameter changed
            // not implemented
            break;
         case 'T':   // Error texts
            this->parseErrorText(command);
            break;
         case 'U':   // Last errors
            this->parseLastError(command);
            break;
         case 'Z':   // End of block
            this->parseEndOfBlock(command);
            break;
         case 'H':   // meaning unknown
         case 'K':   // meaning unknown
         case 'L':   // meaning unknown
         case 'M':   // meaning unknown
         case 'S':   // meaning unknown
         case 'W':   // meaning unknown
            break;
         default:
            LOG_info
               << "Unknown command `"
               << (::isprint(command[0]) ? (char) command[0] : '.')
               << (::isprint(command[1]) ? (char) command[1] : '.')
               << std::hex
               << "' (0x" << std::setfill('0') << std::setw(2) << (unsigned int) command[0]
               << " 0x" << std::setfill('0') << std::setw(2) << (unsigned int) command[1]
               << std::dec
               << "), " << (int) command[2] << " bytes" << std::endl;
            break;
      }
   } else {
      LOG_info
         << "Unknown command `"
         << (::isprint(command[0]) ? (char) command[0] : '.')
         << (::isprint(command[1]) ? (char) command[1] : '.')
         << std::hex
         << "' (0x" << std::setfill('0') << std::setw(2) << (unsigned int) command[0]
         << " 0x" << std::setfill('0') << std::setw(2) << (unsigned int) command[1]
         << std::dec
         << "), " << (int) command[2] << " bytes" << std::endl;
   }
}

/**
 * Handles the `M1' message.
 *
 * @param command The message data.
 */
void Surveillance::parseMeasurement(const uint8_t *command)
{
   if (this->state == ST_RB_SENT) {
      if (command[2] != this->parameterNames.size()*2) {
         this->state = ST_ERROR;
         return;
      }

      std::list<VALUE_DATA> values;
      for (int i = 0; i < command[2] / 2; ++i) {
         int16_t value = (((uint16_t) command[3 + i*2]) << 8) | command[3 + i*2 + 1];

         PARAMETER_NAME parameterTypeInfo = this->parameterNames[i];

         switch (parameterTypeInfo.type) {
            case PNT_STRING:
            {
               uint16_t mbType = parameterTypeInfo.index;

               auto stringIter = this->displayTexts[mbType].find(value);
               if (stringIter != this->displayTexts[mbType].end()) {
                  VALUE_DATA valueData;

                  valueData.index = i;
                  valueData.name = parameterTypeInfo.name;
                  valueData.value = stringIter->second;
                  valueData.rawValue = value;

                  values.push_back(valueData);
               }
               break;
            }
            case PNT_VALUE:
            {
               uint16_t mcIndex = parameterTypeInfo.index;

               auto dataformatIter = this->dataformats.find(mcIndex);
               if (dataformatIter != this->dataformats.end()) {
                  DATAFORMATS dataformat = dataformatIter->second;

                  double dblValue = value;
                  dblValue /= dataformat.divisor;

                  std::ostringstream stringValueStream;
                  stringValueStream << std::fixed << std::setprecision(dataformat.decimals) << dblValue;
                  stringValueStream << dataformat.unit;

                  VALUE_DATA valueData;

                  valueData.index = i;
                  valueData.name = parameterTypeInfo.name;
                  valueData.value = stringValueStream.str();
                  valueData.rawValue = value;

                  values.push_back(valueData);
               }
               break;
            }
            default:
               break;
         }
      }

      for (auto iter = values.begin();
           iter != values.end();
           ++iter) {
         LOG_debug
            << std::hex
            << std::setw(2) << std::setfill('0') << iter->index
            << std::dec
            << " [" << iter->name << "] = [" << iter->value << "] ("
            << (int) iter->rawValue << ")" << std::endl;
      }

      this->handler.handleMeasurement(*this, values);
   }
}

void Surveillance::parseDateTime(const uint8_t *command)
{
   if (command[2] != 7) {
      LOG_error << "date/time message length invalid." << std::endl;
      this->state = ST_ERROR;
      return;
   }

   uint16_t year = 2000 + (command[9] >> 4) * 10 + (command[9] & 0x0f);
   uint8_t month = (command[7] >> 4) * 10 + (command[7] & 0x0f);
   uint8_t day = (command[6] >> 4) * 10 + (command[6] & 0x0f);
   uint8_t dow = command[8];
   uint8_t hour = (command[5] >> 4) * 10 + (command[5] & 0x0f);
   uint8_t minute = (command[4] >> 4) * 10 + (command[4] & 0x0f);
   uint8_t second = (command[3] >> 4) * 10 + (command[3] & 0x0f);

   LOG_debug
      << "Date/Time: "
      << std::dec
      << std::setw(4) << std::setfill('0') << (int) year << "-"
      << std::setw(2) << std::setfill('0') << (int) month << "-"
      << std::setw(2) << std::setfill('0') << (int) day << ", "
      << std::setw(2) << std::setfill('0') << (int) hour << ":"
      << std::setw(2) << std::setfill('0') << (int) minute << ":"
      << std::setw(2) << std::setfill('0') << (int) second << std::endl;
   this->handler.handleTime(*this,
                            dow,
                            year, month, day,
                            hour, minute, second);

   if (this->state == ST_RA_SENT) {
      LOG_trace << "Sending 'Rb' command" << std::endl;
      if (this->fd.send_cmd((uint8_t *) "Rb", (uint8_t *) "\0\0\0", 3) < 0) {
         ::perror("Failed to send Rb command");
         this->state = ST_ERROR;
         return;
      }
      this->state = ST_RB_SENT;
   }
}

void Surveillance::parseFailure(const uint8_t *command)
{
   dump_string("! ", command, 3+command[2], LOG_fatal);
}

void Surveillance::parseParameterNames(const uint8_t *command)
{
   if (command[2] < 5) {
      this->state = ST_ERROR;
      return;
   }

   PARAMETER_NAME entry;

   switch(command[3]) {
      case 'S':
         entry.type = PNT_STRING;
         break;
      case 'I':
         entry.type = PNT_VALUE;
         break;
      default:
         entry.type = PNT_OTHER;
         break;
   }
   entry.index = (((uint16_t) command[4]) << 8) | command[5];
   entry.unknown = (((uint16_t) command[6]) << 8) | command[7];
   entry.name = cp850toUTF8(command+8, command[2]-5);

   this->parameterNames.push_back(entry);
}

void Surveillance::parseDisplayTexts(const uint8_t *command)
{
   if (command[2] < 4) {
      this->state = ST_ERROR;
      return;
   }

   uint16_t page = (((uint16_t) command[3]) << 8) | command[4];
   uint16_t messageID = (((uint16_t) command[5]) << 8) | command[6];

   this->displayTexts[page][messageID] = cp850toUTF8(command+7, command[2]-4);
}

void Surveillance::parseDataformats(const uint8_t *command)
{
   if (command[2] != 8) {
      this->state = ST_ERROR;
      return;
   }

   uint16_t index = (((uint16_t) command[3]) << 8) | command[4];

   DATAFORMATS dataformat;
   dataformat.unit = cp850toUTF8(command+5, 1);
   dataformat.decimals = command[6];
   dataformat.divisor = (((uint16_t) command[7]) << 8) | command[8];
   dataformat.unknown = (((uint16_t) command[9]) << 8) | command[10];

   if (dataformat.unit == " ") {
      dataformat.unit = "";
   }

   this->dataformats[index] = dataformat;
}

void Surveillance::parseSettingsText(const uint8_t *command)
{
}

void Surveillance::parseFormats(const uint8_t *command)
{
   if (command[2] != 17) {
      this->state = ST_ERROR;
      return;
   }

   uint16_t paramID = (((uint16_t) command[3]) << 8) | command[4];

   PARAMETER_FORMAT format;
   format.unit = cp850toUTF8(command+5, 1);
   format.comma = command[6];
   format.factor = (((uint16_t) command[7]) << 8) | command[8];
   format.min = (((uint16_t) command[9]) << 8) | command[10];
   format.max = (((uint16_t) command[11]) << 8) | command[12];
   format.std = (((uint16_t) command[13]) << 8) | command[14];
   format.value = (((uint16_t) command[18]) << 8) | command[19];

   parameterFormats[paramID] = format;
}

void Surveillance::parseOperationMode(const uint8_t *command)
{
   if (command[2] < 1) {
      this->state = ST_ERROR;
      return;
   }

   uint16_t modeID = (((uint16_t) command[3]) << 8) | command[4];

   this->operationModes[modeID] = cp850toUTF8(command+5, command[2]-2);
}

void Surveillance::parseWeeklyProgram(const uint8_t *command)
{
}

void Surveillance::parseErrorText(const uint8_t *command)
{
   if (command[2] < 1) {
      this->state = ST_ERROR;
      return;
   }

   uint8_t errorID = command[3];

   this->errorMessages[errorID] = cp850toUTF8(command+4, command[2]-1);
}

void Surveillance::parseLastError(const uint8_t *command)
{
   if (command[2] != 10) {
      this->state = ST_ERROR;
      return;
   }

   ERROR_EVENT event;
   event.errorID = command[3];
   event.unknown1 = command[4];
   event.unknown2 = command[5];
   event.second = this->bcd(command[6]);
   event.minute = this->bcd(command[7]);
   event.hour = this->bcd(command[8]);
   event.day = this->bcd(command[9]);
   event.month = this->bcd(command[10]);
   event.year = this->bcd(command[12]);
   event.dow = command[11];

   this->errorEvents.push_back(event);
}

void Surveillance::parseEndOfBlock(const uint8_t *command)
{
   if (command[2] != 1) {
      this->state = ST_ERROR;
      return;
   }

   if (debug_level) {
      switch (command[3]) {
         case 'A':
         {
            LOG_debug << "M1 data:" << std::endl;
            for (auto iter = this->parameterNames.begin();
                 iter != this->parameterNames.end();
                 ++iter) {
               if (iter->type == PNT_STRING) {
                  LOG_debug << "[STRING]" << std::endl;
               } else {
                  LOG_debug << "[" << iter->name << "]" << std::endl;
               }
            }

            break;
         }
         case 'B':
         {
            LOG_debug << "Display texts:" << std::endl;
            for (auto pageIter = this->displayTexts.begin();
                 pageIter != this->displayTexts.end();
                 ++pageIter) {
               for (auto messageIter = pageIter->second.begin();
                    messageIter != pageIter->second.end();
                    ++messageIter) {

                  LOG_debug
                     << std::hex
                     << "0x" << std::setfill('0') << std::setw(2) << (unsigned int) pageIter->first
                     << "/"
                     << "0x" << std::setfill('0') << std::setw(2) << (unsigned int) messageIter->first
                     << std::dec
                     << " ("
                     << std::setw(3) << (unsigned int) pageIter->first
                     << "/"
                     << std::setw(3) << (unsigned int) messageIter->first
                     << "): "
                     << messageIter->second
                     << std::endl;
               }
            }
            break;
         }
         case 'C':
         {
            LOG_debug << "Data formats:" << std::endl;
            for (int i = 0; i < this->dataformats.size(); ++i) {
               DATAFORMATS dataformat = this->dataformats[i];

               LOG_debug
                  << i << ": "
                  << "Unit: [" << dataformat.unit << "], "
                  << "Decimals: " << (unsigned int) dataformat.decimals << ", "
                  << "Divisor: " << (unsigned int) dataformat.divisor << ", "
                  << "Unknown: " << (unsigned int) dataformat.unknown
                  << std::endl;
            }
            break;
         }
         case 'F':
            LOG_debug << "Operation modes:" << std::endl;
            for (auto operationModeIter = this->operationModes.begin();
                 operationModeIter != this->operationModes.end();
                 ++operationModeIter) {
               LOG_debug
                  << std::hex
                  << "0x" << std::setfill('0') << std::setw(2) << (unsigned int) operationModeIter->first
                  << std::dec
                  << " (" << std::setw(3) << (unsigned int) operationModeIter->first << "): "
                  << operationModeIter->second << std::endl;
            }
            break;
         case 'T':
            LOG_debug << "Errorcodes:" << std::endl;
            for (auto errorMessageIter = this->errorMessages.begin();
                 errorMessageIter != this->errorMessages.end();
                 ++errorMessageIter) {
               LOG_debug
                  << std::hex
                  << "0x" << std::setfill('0') << std::setw(2) << (unsigned int) errorMessageIter->first
                  << std::dec
                  << " (" << std::setw(3) << (unsigned int) errorMessageIter->first << "): "
                  << errorMessageIter->second << std::endl;
            }
            break;
         case 'U':
            LOG_info << "Last errors:" << std::endl;
            for (auto iter = this->errorEvents.begin();
                 iter != this->errorEvents.end();
                 ++iter) {
               auto errorText = this->errorMessages.find(iter->errorID);
               std::string description;
               if (errorText != this->errorMessages.end()) {
                  description = errorText->second;
               } else {
                  description = "Unknown";
               }

               LOG_info
                  << std::setfill('0') << std::setw(4) << 2000+(unsigned int) iter->year << "-"
                  << std::setfill('0') << std::setw(2) << (unsigned int) iter->month << "-"
                  << std::setfill('0') << std::setw(2) << (unsigned int) iter->day << ", "
                  << std::setfill('0') << std::setw(2) << (unsigned int) iter->hour << ":"
                  << std::setfill('0') << std::setw(2) << (unsigned int) iter->minute << ":"
                  << std::setfill('0') << std::setw(2) << (unsigned int) iter->second << " - "
                  << description << std::endl;
            }
            break;
      }
   }
}

int Surveillance::bcd(uint8_t bcdValue) {
   return (bcdValue >> 4) * 10 + (bcdValue & 0x0f);
}

}
