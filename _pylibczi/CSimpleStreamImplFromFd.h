//
// Created by Jamie Sherman on 11/21/19.
//

#ifndef _PYLIBCZI_CSIMPLESTREAMIMPLFROMFD_H
#define _PYLIBCZI_CSIMPLESTREAMIMPLFROMFD_H

#include <cstdio>
#include <cstdint>
#include "inc_libCZI.h"

namespace pb_helpers {
/*!
 * @brief This class takes a file descriptor and creates a new/duplicate file descriptor from it. The new
 * file descriptor is then used to open a FILE * and subsequently used to access the file date. When this
 * object goes out of scope the destructor calls fclose. fclose then closes the FILE * as well as the duplicate
 * file descriptor.
 */
  class CSimpleStreamImplFromFd: public libCZI::IStream {
  private:
      FILE* m_fp;
  public:
      CSimpleStreamImplFromFd() = delete;
      explicit CSimpleStreamImplFromFd(int file_descriptor_);
      ~CSimpleStreamImplFromFd() override { fclose(m_fp); };
      void Read(std::uint64_t offset_, void* data_ptr_, std::uint64_t size_, std::uint64_t* bytes_read_ptr_) override;
  };
}

#endif //_PYLIBCZI_CSIMPLESTREAMIMPLFROMFD_H
