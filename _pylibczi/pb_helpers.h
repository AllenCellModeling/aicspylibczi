#ifndef _PYLIBCZI_PB_HELPERS_H
#define _PYLIBCZI_PB_HELPERS_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <memory>
#include <vector>
#include <set>
#include <iostream>

#include "Image.h"
#include "ImageFactory.h"
#include "Reader.h"
#include "exceptions.h"

namespace py=pybind11;
namespace pb_helpers {

  py::array packArray(pylibczi::ImageVector& images_);
  py::list *packStringArray(pylibczi::SubblockMetaVec& metadata_);

  template<typename T>
  py::array* makeArray(unsigned long size_, std::vector<ssize_t>& shape_, pylibczi::ImageVector& images_)
  {
      if (size_==0) return new py::array_t<T>({1}, new T);
      T* ptr;
      try {
          ptr = new T[size_];
      }
      catch (std::bad_alloc& ba) {
          std::cout << ba.what() << std::endl;
          throw pylibczi::ImageCopyAllocFailed("try using a more constraints (S=1, T=5, etc on the DimensionIndex).", size_);
      }

      T* position = ptr;
      for (const auto& image : images_) {
          auto typedImage = pylibczi::ImageFactory::getDerived<T>(image);
          size_t length = typedImage->length();
          std::copy(typedImage->getRawPtr(), typedImage->getRawPtr()+length, position);
          position += length;
      }

      py::capsule freeWhenDone(ptr, [](void* f_) {
          T* ptr = reinterpret_cast<T*>(f_);
          delete[] ptr;
      });

      return new py::array_t<T>(shape_, ptr, freeWhenDone);
  }

}

#endif //_PYLIBCZI_PB_HELPERS_H
