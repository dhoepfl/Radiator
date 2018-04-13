#ifndef __DH_OUTPUT_H__
#define __DH_OUTPUT_H__

#include "surveillance.h"

#include <ostream>

namespace radiator {

class OutputHandler : public radiator::SurveillanceHandler {
protected:
   std::basic_ostream<char> *ostream;

public:
   OutputHandler(std::string filename);
   virtual ~OutputHandler();

   virtual void handleTime(radiator::Surveillance &surveillance,
                           uint8_t dow,
                           uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t minute, uint8_t second);

   virtual void handleMeasurement(Surveillance &surveillance,
                                  std::list<VALUE_DATA> values);

   virtual void handleError(Surveillance &surveillance,
                            uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second,
                            std::string description);
};

}

#endif
