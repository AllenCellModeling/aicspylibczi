#include "StreamImplLockingRead.h"
#include <ios>
#include <thread>

namespace pylibczi {

StreamImplLockingRead::StreamImplLockingRead(const wchar_t* filename)
{
  this->infile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
#if defined(_WIN32)
  this->infile.open(filename, std::ios::binary | std::ios::in);
#else
  // convert the wchar_t to an UTF8-string
  size_t requiredSize = std::wcstombs(nullptr, filename, 0);
  std::string conv(requiredSize, 0);
  conv.resize(std::wcstombs(&conv[0], filename, requiredSize));
  this->infile.open(conv.c_str(), std::ios::binary | std::ios::in);
#endif
}

StreamImplLockingRead::~StreamImplLockingRead()
{
  this->infile.close();
}

void
StreamImplLockingRead::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
  std::unique_lock<std::mutex> lck(m_mutex);
  this->infile.seekg(offset, std::ios::beg);
  this->infile.read((char*)pv, size);
  if (ptrBytesRead != nullptr) {
    *ptrBytesRead = this->infile.gcount();
    // assert(*ptrBytesRead==size);
  }
}

}
