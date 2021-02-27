#ifndef _AICSPYLIBCZI_DIMINDEX_H
#define _AICSPYLIBCZI_DIMINDEX_H

#include <cstdint>
#include <cstdlib>

#include "inc_libCZI.h"

namespace pylibczi {

/*!
 * @brief This class is a convienience class to allow me to deal with X & Y as
 * well as the libCZI::DimensionIndexs
 */
enum class DimIndex : std::uint16_t
{
  invalid = 0, ///< Invalid dimension index.

  MinDim = 1, ///< This enum must be have the value of the lowest (valid)
              ///< dimension index.

  A = 1,  ///< The Sample-dimension -> BGR
  X = 2,  ///< The X-dimension.  ** NOT in libCZI::DimensionIndex.
  Y = 3,  ///< The Y-dimension.  ** NOT in libCZI::DimensionIndex.
  Z = 4,  ///< The Z-dimension.
  C = 5,  ///< The C-dimension ("channel").
  T = 6,  ///< The T-dimension ("time").
  R = 7,  ///< The R-dimension ("rotation").
  M = 8,  ///< The m_index.    ** NOT in libCZI::DimensionIndex.
  S = 9,  ///< The S-dimension ("scene").
  I = 10, ///< The I-dimension ("illumination").
  H = 11, ///< The H-dimension ("phase").
  V = 12, ///< The V-dimension ("view").
  B = 13, ///< The B-dimension ("block") - its use is deprecated.

  MaxDim = 13 ///< This enum must be have the value of the highest (valid)
              ///< dimension index.
};

/*!
 * @brief map the DimIndex definded above to their corresponding characters
 * @param index_ the DimIndex to be mapped to a character
 * @return the character the index_ maps to or ? if undefined.
 */
char
dimIndexToChar(DimIndex index_);

DimIndex
charToDimIndex(char c_);

DimIndex
dimensionIndexToDimIndex(libCZI::DimensionIndex index_);

libCZI::DimensionIndex
dimIndexToDimensionIndex(DimIndex dim_index_);

}

#endif //_AICSPYLIBCZI_DIMINDEX_H
