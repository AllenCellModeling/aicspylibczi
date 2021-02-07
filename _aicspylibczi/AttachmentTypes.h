//
// Created by Jamie Sherman on 2/6/21.
//

#ifndef _AICSPYLIBCZI_ATTACHMENTTYPES_H
#define _AICSPYLIBCZI_ATTACHMENTTYPES_H

#include "inc_libCZI.h"

namespace pylibczi {




class EventListEntery
{
  int32_t m_eventType;
  double m_time;
  std::string m_description;

public:
  EventListEntery(std::shared_ptr<libCZI::IStream> eStream_, uint64_t &startFrom_);

};

class EventLists
{
  int32_t m_messageSize;
  int32_t m_numberOfEvents;
  std::vector<EventListEntery> m_events;
public:
  EventLists(std::shared_ptr<libCZI::IStream> eStream_, uint64_t &startFrom_);

};

class TimeStamps
{
  std::vector<double> m_timestamps;

public:
  TimeStamps(std::shared_ptr<libCZI::IStream> eStream_, uint64_t &startFrom_);

};

} // pylibczi

#endif //_AICSPYLIBCZI_ATTACHMENTTYPES_H
