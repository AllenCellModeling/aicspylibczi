#ifndef _PYLIBCZI_CONSTANTS_H
#define _PYLIBCZI_CONSTANTS_H

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <cstdint>

#include "inc_libCZI.h"

namespace pylibczi{

  struct Constants {
      using CziDi = libCZI::DimensionIndex;
      static const std::initializer_list<CziDi> s_sortOrder;

      bool dimsMatch(const libCZI::CDimCoordinate& target_dims_, const libCZI::CDimCoordinate& czi_dims_);
  };
}

#endif //_PYLIBCZI_CONSTANTS_H
