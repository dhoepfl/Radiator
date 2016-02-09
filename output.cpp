#include "output.h"

#include "debug.h"

#include <fstream>
#include <iostream>
#include <iomanip>

radiator::OutputHandler::OutputHandler(std::string filename)
{
   if (filename == "-") {
      ostream = &std::cout;
   } else if (filename.length() > 0) {
      ostream = new std::ofstream(filename);
   } else {
      ostream = NULL;
   }
}

radiator::OutputHandler::~OutputHandler()
{
   if (ostream != &std::cout) {
      delete ostream;
   }
}

void radiator::OutputHandler::handleTime(radiator::Surveillance &surveillance,
                                         uint8_t dow,
                                         uint16_t year, uint8_t month, uint8_t day,
                                         uint8_t hour, uint8_t minute, uint8_t second)
{
   if (!ostream) {
      return;
   }

   static const char *dowString[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

   *ostream << "[TIME] "
      << dowString[dow-1] << ", "
      << std::dec
      << std::setw(4) << std::setfill('0') << (int) year << "-"
      << std::setw(2) << std::setfill('0') << (int) month << "-"
      << std::setw(2) << std::setfill('0') << (int) day << ", "
      << std::setw(2) << std::setfill('0') << (int) hour << ":"
      << std::setw(2) << std::setfill('0') << (int) minute << ":"
      << std::setw(2) << std::setfill('0') << (int) second << std::endl;
}

void radiator::OutputHandler::handleMeasurement(Surveillance &surveillance,
                                                std::list<VALUE_DATA> values)
{
   if (!ostream) {
      return;
   }

   auto parameterNames = surveillance.getParameterNames();
   for (auto iter = values.begin();
        iter != values.end();
        ++iter)
   {
      *ostream
         << "[VALUE] "
         << std::dec << std::setw(2) << std::setfill('0') << iter->index << std::dec
         << " [" << iter->name << "] = [" << iter->value << "]";

      switch(parameterNames[iter->index].type) {
         case PNT_STRING:
            *ostream << " [S]";
            break;
         case PNT_VALUE:
            *ostream << " [N]";
            break;
         default:
            *ostream << " [O]";
            break;
      }

      *ostream
         << " (" << (int) iter->rawValue << ")" << std::endl;
   }
   *ostream << std::endl;
}
