#include <cstdio>
#include <mutex>
#include <pybind11/pybind11.h>
#include "inc_libCZI.h"

#ifndef _AICSPYLIBCZI_REMOTEISTREAM_H
#define _AICSPYLIBCZI_REMOTEISTREAM_H

namespace py = pybind11;

namespace pb_helpers{

class RemoteIStream : public libCZI::IStream
{
  py::object m_abstractBufferedFile;
  std::mutex m_mutex;

public:
  explicit RemoteIStream(py::handle abstractBufferedFile)
  {
    std::cout << "reinterpret IN" << std::endl;
    m_abstractBufferedFile = py::reinterpret_borrow<py::object>(abstractBufferedFile);
    std::cout << "reinterpret OUT" << std::endl;
    std::string msg = (PyGILState_Check() ? "RIS has GIL" : "GIL no acquired!");
    std::cout << msg << std::endl
  }

  void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;

  ~RemoteIStream(){}

};

}

#endif //_AICSPYLIBCZI_REMOTEISTREAM_H
