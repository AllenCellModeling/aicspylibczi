#ifndef _PYLIBCZI_SUBBLOCKSORTABLE_H
#define _PYLIBCZI_SUBBLOCKSORTABLE_H

#include <utility>
#include <vector>

#include "constants.h"
#include "inc_libCZI.h"

namespace pylibczi {

class SubblockSortable
{
protected:
  libCZI::CDimCoordinate m_planeCoordinate;
  libCZI::PixelType m_pixelType;
  int m_indexM;
  bool m_isMosaic;

public:
  SubblockSortable(const libCZI::CDimCoordinate* plane_,
                   int index_m_,
                   bool is_mosaic_ = false,
                   libCZI::PixelType pixel_type_ = libCZI::PixelType::Invalid)
    : m_planeCoordinate(plane_)
    , m_pixelType(pixel_type_)
    , m_indexM(index_m_)
    , m_isMosaic(is_mosaic_)
  {}

  virtual ~SubblockSortable() {}

  const libCZI::CDimCoordinate* coordinatePtr() const { return &m_planeCoordinate; }

  libCZI::CDimCoordinate cDims() const { return m_planeCoordinate; }

  int mIndex() const { return m_indexM; }

  libCZI::PixelType pixelType(void) const { return m_pixelType; }

  std::map<char, size_t> getDimsAsChars() const
  {
    return SubblockSortable::getValidIndexes(m_planeCoordinate, m_indexM, m_isMosaic);
  }

  static std::map<char, size_t> getValidIndexes(const libCZI::CDimCoordinate& planecoord_,
                                                int index_m_,
                                                bool is_mosaic_ = false)
  {
    std::map<char, size_t> ans;
    for (auto di : Constants::s_sortOrder) {
      int value;
      if (planecoord_.TryGetPosition(di, &value))
        ans.emplace(libCZI::Utils::DimensionToChar(di), value);
    }
    if (is_mosaic_)
      ans.emplace('M', index_m_);
    return ans;
  }

  std::map<char, size_t> getValidIndexes(bool is_mosaic_ = false) const
  {
    return SubblockSortable::getValidIndexes(m_planeCoordinate, m_indexM, is_mosaic_);
  }

  bool operator<(const SubblockSortable& other_) const
  {
    if (!m_isMosaic || m_indexM == -1 || other_.m_indexM == -1)
      return SubblockSortable::aLessThanB(m_planeCoordinate, other_.m_planeCoordinate);
    return SubblockSortable::aLessThanB(m_planeCoordinate, m_indexM, other_.m_planeCoordinate, other_.m_indexM);
  }

  bool operator==(const SubblockSortable& other_) const { return !(*this < other_) && !(other_ < *this); }

  static bool aLessThanB(const libCZI::CDimCoordinate& a_, const libCZI::CDimCoordinate& b_)
  {
    for (auto di : Constants::s_sortOrder) {
      int aValue, bValue;
      if (a_.TryGetPosition(di, &aValue) && b_.TryGetPosition(di, &bValue) && aValue != bValue)
        return aValue < bValue;
    }
    return false;
  }

  static bool aLessThanB(const libCZI::CDimCoordinate& a_,
                         const int a_index_m_,
                         const libCZI::CDimCoordinate& b_,
                         const int b_index_m_)
  {
    if (!aLessThanB(a_, b_) && !aLessThanB(b_, a_))
      return a_index_m_ < b_index_m_;
    return aLessThanB(a_, b_);
  }
};
}

#endif //_PYLIBCZI_SUBBLOCKSORTABLE_H
