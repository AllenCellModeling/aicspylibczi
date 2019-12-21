#ifndef _AICSPYLIBCZI_DIMINDEX_H
#define _AICSPYLIBCZI_DIMINDEX_H

#include <cstdlib>
#include <cstdint>

#include "inc_libCZI.h"

namespace pylibczi {

  /*!
   * @brief This class is a convienience class to allow me to deal with X & Y as well as the libCZI::DimensionIndexs
   */
  enum class DimIndex: std::uint8_t {
      invalid = 0,    ///< Invalid dimension index.

      MinDim = 1,        ///< This enum must be have the value of the lowest (valid) dimension index.

      X = 1,          ///< The X-dimension.  ** NOT in libCZI::DimensionIndex.
      Y = 2,          ///< The Y-dimension.  ** NOT in libCZI::DimensionIndex.
      Z = 3,            ///< The Z-dimension.
      C = 4,            ///< The C-dimension ("channel").
      T = 5,            ///< The T-dimension ("time").
      R = 6,            ///< The R-dimension ("rotation").
      M = 7,            ///< The m_index.    ** NOT in libCZI::DimensionIndex.
      S = 8,            ///< The S-dimension ("scene").
      I = 9,            ///< The I-dimension ("illumination").
      H = 10,            ///< The H-dimension ("phase").
      V = 11,            ///< The V-dimension ("view").
      B = 12,            ///< The B-dimension ("block") - its use is deprecated.

      MaxDim = 12    ///< This enum must be have the value of the highest (valid) dimension index.
  };

  /*!
   * @brief map the DimIndex definded above to their corresponding characters
   * @param index_ the DimIndex to be mapped to a character
   * @return the character the index_ maps to or ? if undefined.
   */
  char dimIndexToChar(DimIndex index_);
  DimIndex charToDimIndex(char c_);
  libCZI::DimensionIndex dimIndexToDimensionIndex(DimIndex index_);
  DimIndex dimensionIndexToDimIndex(libCZI::DimensionIndex index_);

}

#endif //_AICSPYLIBCZI_DIMINDEX_H
