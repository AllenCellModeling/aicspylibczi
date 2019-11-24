//
// Created by Jamie Sherman on 11/21/19.
//

#include <fcntl.h>

#include "CSimpleStreamImplFromFd.h"
#include "exceptions.h"

#ifdef _WIN32
#include <io.h>
#include <stdio.h>
#else
#include <unistd.h>
#endif

namespace pb_helpers {

  CSimpleStreamImplFromFd::CSimpleStreamImplFromFd(int file_descriptor_)
      :libCZI::IStream()
  {
#ifdef _WIN32
      int dupDesc = _dup(file_descriptor_);
      m_fp = _fdopen(dupDesc, "r");
#else
      int dupDesc = dup(file_descriptor_);
      m_fp = fdopen(dupDesc, "r");
#endif
      if (m_fp==nullptr)
          throw pylibczi::FilePtrException("Reader class received a bad FILE *!");
  }

  void
  CSimpleStreamImplFromFd::Read(std::uint64_t offset_, void* data_ptr_, std::uint64_t size_, std::uint64_t* bytes_read_ptr_)
  {
#ifdef _WIN32
      _fseeki64(this->m_fp, offset_, SEEK_SET);
#else
      fseeko(this->m_fp, offset_, SEEK_SET);
#endif
      std::uint64_t bytesRead = fread(data_ptr_, 1, (size_t) size_, this->m_fp);
      if (bytes_read_ptr_!=nullptr)
          (*bytes_read_ptr_) = bytesRead;
  }

}
