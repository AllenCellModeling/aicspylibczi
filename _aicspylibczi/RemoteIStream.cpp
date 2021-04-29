#include <iostream>

#include "RemoteIStream.h"

namespace pb_helpers{



void
RemoteIStream::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
{
  try {
    std::cout << "in READ" << std::endl;
    std::unique_lock<std::mutex> lck(m_mutex);
    std::cout << "about to Seek -- offset = " << offset << " size = " << size << std::endl;
    m_abstractBufferedFile.attr("seek")(offset, 0);
    std::cout << "about to READ" << std::endl;
    py::object result = m_abstractBufferedFile.attr("read")(size);
    char* buffer;
    ssize_t length;
    std::cout << "about to Convert" << std::endl;
    if (PYBIND11_BYTES_AS_STRING_AND_SIZE(result.ptr(), &buffer, &length)) {
      std::cout << "python string conversion failed!" << std::endl;
      throw std::runtime_error("unable to convert python bytes to char *");
    }
    memcpy(pv, buffer, size);
    std::cout << "Read Successful -- length = " << length << std::endl;
    *ptrBytesRead = size;
  }catch(std::exception &e){
    std::cout << "exception reading file: " << e.what() << std::endl;
    throw std::runtime_error("reading failed");
  }

}

}
