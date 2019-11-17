#ifndef _PYLIBCZI_INDEXMAP_H
#define _PYLIBCZI_INDEXMAP_H

#include <map>
#include <utility>

#include "inc_libCZI.h"

namespace pylibczi {

  /*!
   * IndexMap is used to store the SubblockInfo. It holds onto the original subblockindex (it's order in the file) and it's postion in the
   * vector. This is encase I need to internally shuffle things one way or another. It's likely overkill and only really becomes useful
   * if the sort order is changed to be something other than the STCZYX. In normal sort order it has no real effect.
   */
  class IndexMap {
      typedef std::map<libCZI::DimensionIndex, int> MapType;
      int m_subBlockIndex; // the subBlock index from the file
      int m_indexM; // the mIndex
      int m_position; // the index of the subBlock in the file within the subset included
      MapType m_dims;

      static const std::vector<libCZI::DimensionIndex> s_sortOrder;

  public:
      IndexMap(int index_, const libCZI::SubBlockInfo& info_);

      IndexMap()
          :m_subBlockIndex(), m_indexM(), m_position(), m_dims() { }

      bool operator<(const IndexMap& other_);

      bool lessThanSubBlock(const IndexMap& other_) const { return this->m_subBlockIndex<other_.m_subBlockIndex; }

      bool isMIndexValid() const;

      int mIndex() const { return m_indexM; }

      void position(int x_) { m_position = x_; }

      MapType dimIndex()
      {
          return m_dims;
      }

  };

}

#endif //_PYLIBCZI_INDEXMAP_H
