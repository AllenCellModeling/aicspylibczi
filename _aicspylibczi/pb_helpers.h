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

  py::array packArray(pylibczi::ImagesContainerBase::ImagesContainerBasePtr &base_ptr_);
  py::list *packStringArray(pylibczi::SubblockMetaVec& metadata_);
  std::vector<std::pair<char, size_t> > getAndFixShape(pylibczi::ImagesContainerBase *bptr_);

  template <typename T>
  py::array *memoryToNpArray(pylibczi::ImagesContainerBase *bptr_, std::vector<std::pair<char, size_t> > &charSizes_){
      pylibczi::ImagesContainer<T>* tptr = bptr_->getBaseAsTyped<T>();

      std::vector<ssize_t> shape(charSizes_.size(), 0);
      std::transform(charSizes_.begin(), charSizes_.end(), shape.begin(), [](const std::pair<char, size_t>& a_) {
          return a_.second;
      });

      T* mptr = tptr->releaseMemory();

      py::capsule freeWhenDone(mptr, [](void* f_) {
          T* ptr = reinterpret_cast<T*>(f_);
          delete[] ptr;
      });

      return new py::array_t<T>(shape, mptr, freeWhenDone);
  }

}

#endif //_PYLIBCZI_PB_HELPERS_H
