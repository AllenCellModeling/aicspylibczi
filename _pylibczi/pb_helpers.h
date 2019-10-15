//
// Created by Jamie Sherman on 2019-09-11.
//

#ifndef _PYLIBCZI__PYLIBCZI_PB_HELPERS_H
#define _PYLIBCZI__PYLIBCZI_PB_HELPERS_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <set>
#include <iostream>
#include "Image.h"
#include "Reader.h"
#include "exceptions.h"

namespace py=pybind11;
namespace pb_helpers {

  using ImVec = pylibczi::Image::ImVec;

  template<typename T>
  py::array* make_array(int new_size, std::vector<ssize_t>& shp, ImVec& imgs);

  py::array pack_array(pylibczi::ImageVector& imgs);

}

#endif //_PYLIBCZI__PYLIBCZI_PB_HELPERS_H
