//
// Created by Jamie Sherman on 8/13/20.
//

#ifndef _AICSPYLIBCZI_STREAMIMPLLOCKINGREAD_H
#define _AICSPYLIBCZI_STREAMIMPLLOCKINGREAD_H

#include <cstdint>
#include <cstdio>
#include <mutex>

#include "inc_libCZI.h"

namespace pylibczi {
/// <summary>	A simplistic stream implementation (based on C++ streams). Note that this implementation is NOT
/// thread-safe.</summary>
class StreamImplLockingRead : public libCZI::IStream
{
private:
  std::ifstream infile;
  std::mutex m_mutex;

public:
  StreamImplLockingRead() = delete;

  explicit StreamImplLockingRead(const wchar_t* filename);

  ~StreamImplLockingRead() override;

  void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
};

}

#endif //_AICSPYLIBCZI_STREAMIMPLLOCKINGREAD_H
