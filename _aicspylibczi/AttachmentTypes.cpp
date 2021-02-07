//
// Created by Jamie Sherman on 2/6/21.
//

#include "AttachmentTypes.h"
#include <iostream>

namespace pylibczi{

EventListEntery::EventListEntery(std::shared_ptr<libCZI::IStream> eStream_, uint64_t & startFrom_): m_eventType(0), m_time(0.0)
{
  uint64_t bytesRead;
  int32_t eSize = 0;
  eStream_->Read(startFrom_, &eSize, 4, &bytesRead);
  eStream_->Read(startFrom_ + 4, &m_time, 8, &bytesRead );
  eStream_->Read(startFrom_ + 12, &m_eventType, 4, &bytesRead);
  char *description = new char[eSize - 16];
  eStream_->Read(startFrom_ + 16, description, eSize - 16, &bytesRead);
  m_description = std::string(description);
  std::cout << eSize << " | " << m_time << " | " << m_eventType << std::endl;
  std::cout << "Desciption: " << m_description << std::endl;
  startFrom_ += eSize;
  delete[] description;
}

EventLists::EventLists(std::shared_ptr<libCZI::IStream> eStream_, uint64_t& startFrom_)
{
  uint64_t bytesRead;
  eStream_->Read(startFrom_, &m_messageSize, 4, &bytesRead);
  eStream_->Read(startFrom_ + 4, &m_numberOfEvents, 4, &bytesRead);
  startFrom_ += 8;
  m_events.reserve(m_numberOfEvents);
  for(int i = 0; i < m_numberOfEvents; i++)
    m_events.emplace_back(eStream_, startFrom_);
}

TimeStamps::TimeStamps(std::shared_ptr<libCZI::IStream> eStream_, uint64_t &startFrom_)
{
  std::cout << "in timestamps" << std::endl;
  uint64_t bytesRead;
  int32_t eSize = 0;
  int32_t numberOfTimestamps = 0;
  eStream_->Read(startFrom_, &eSize, 4, &bytesRead);
  eStream_->Read(startFrom_ + 4, &numberOfTimestamps, 4, &bytesRead);
  m_timestamps.reserve(numberOfTimestamps);
  std::cout << "Number of timestamps: " << numberOfTimestamps << std::endl;
  double *dtmp = new double[numberOfTimestamps];
  eStream_->Read(startFrom_ + 8, dtmp, numberOfTimestamps*8, &bytesRead );
  std::for_each(dtmp, &(dtmp[numberOfTimestamps]), [&](const double &ts_){
    m_timestamps.emplace_back(ts_);
    std::cout << "Timestamp: " << ts_ << std::endl;
  });
  delete [] dtmp;
  startFrom_ += eSize;
}

}
