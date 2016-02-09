#ifndef __DH_SURVEILLANCE_H__
#define __DH_SURVEILLANCE_H__

#include "device.h"

#include <map>
#include <list>
#include <deque>

namespace radiator {

typedef enum
{
   ST_ERROR = -1,
   ST_STARTING = 0,
   ST_RA_SENT,
   ST_RB_SENT
} STATE;

typedef struct
{
   std::string unit;
   uint8_t comma;
   uint16_t factor;
   uint16_t min;
   uint16_t max;
   uint16_t std;
   uint16_t value;
} PARAMETER_FORMAT;

typedef enum
{
   PNT_STRING,
   PNT_VALUE,
   PNT_OTHER
} PARAMETER_NAME_TYPE;

typedef struct
{
   PARAMETER_NAME_TYPE  type;
   uint16_t             index;
   uint16_t             unknown;
   std::string          name;
} PARAMETER_NAME;

typedef struct
{
   std::string unit;
   uint8_t decimals;
   uint16_t divisor;
   uint16_t unknown;
} DATAFORMATS;

typedef struct
{
   uint8_t errorID;
   uint8_t unknown1;
   uint8_t unknown2;
   uint8_t second;
   uint8_t minute;
   uint8_t hour;
   uint8_t day;
   uint8_t month;
   uint8_t year;
   uint8_t dow;
} ERROR_EVENT;

typedef struct
{
   uint16_t index;
   std::string name;
   std::string value;
   uint16_t rawValue;
} VALUE_DATA;

class Surveillance;

class SurveillanceHandler {
public:
   SurveillanceHandler() {}
   virtual ~SurveillanceHandler() {}

   virtual void handleTime(Surveillance &surveillance,
                           uint8_t dow,
                           uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second) {}

   virtual void handleMeasurement(Surveillance &surveillance,
                                  std::list<VALUE_DATA> values) {}
};

class Surveillance
{
public:
   Surveillance(std::string devicename, int timeout, SurveillanceHandler &handler);
   virtual ~Surveillance();

   void main_loop(void);

   STATE                                     getState() { return this->state; }

   std::deque<PARAMETER_NAME>                getParameterNames() { return this->parameterNames; }

   std::map<int, std::map<int, std::string>> getDisplayTexts() { return this->displayTexts; }
   std::map<int, DATAFORMATS>                getDataformats() { return this->dataformats; }
   std::map<int, std::string>                getOperationModes() { return this->operationModes; }
   std::map<int, PARAMETER_FORMAT>           getParameterFormats() { return this->parameterFormats; }

   std::list<ERROR_EVENT>                    getErrorsEvents() { return this->errorEvents; }
   std::map<int, std::string>                getErrorMessages() { return this->errorMessages; }

protected:
   STATE                                     state;

   std::deque<PARAMETER_NAME>                parameterNames;
   std::map<int, std::map<int, std::string>> displayTexts;
   std::map<int, DATAFORMATS>                dataformats;
   std::map<int, std::string>                operationModes;
   std::map<int, PARAMETER_FORMAT>           parameterFormats;
   std::map<int, std::string>                errorMessages;
   std::list<ERROR_EVENT>                    errorEvents;

private:
   SurveillanceHandler                       &handler;
   Device                                    fd;
   int                                       timeout;

private:
   void handle_command(const uint8_t *command);

   void parseMeasurement(const uint8_t *command);
   void parseDateTime(const uint8_t *command);
   void parseFailure(const uint8_t *command);
   void parseParameterNames(const uint8_t *command);
   void parseDisplayTexts(const uint8_t *command);
   void parseDataformats(const uint8_t *command);
   void parseSettingsText(const uint8_t *command);
   void parseFormats(const uint8_t *command);
   void parseOperationMode(const uint8_t *command);
   void parseWeeklyProgram(const uint8_t *command);
   void parseErrorText(const uint8_t *command);
   void parseLastError(const uint8_t *command);
   void parseEndOfBlock(const uint8_t *command);

   int bcd(uint8_t bcdValue);
};

}

#endif
