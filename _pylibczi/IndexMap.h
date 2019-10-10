//
// Created by Jamie Sherman on 2019-08-17.
//

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
      typedef std::pair<const libCZI::DimensionIndex, int> value_type;
      typedef std::map<libCZI::DimensionIndex, int> map_type;
      int m_subblockIndex; // the subblock index from the file
      int m_index; // the mIndex
      int m_position; // the index of the subblock in the file within the subset included
      map_type m_dims;

      static const std::vector<libCZI::DimensionIndex> m_sortOrder;

  public:
      IndexMap(int idx, const libCZI::SubBlockInfo& info);

      IndexMap()
              :m_subblockIndex(), m_index(), m_position(), m_dims() { }

      bool operator<(const IndexMap& b);

      bool lessThanSubblock(const IndexMap& b) const { return this->m_subblockIndex<b.m_subblockIndex; }

      bool IsMIndexValid() const;

      int mIndex() const { return m_index; }

      void position(int x) { m_position = x; }

      map_type dimIndex()
      {
          return m_dims;
      }

  };

}

#endif //_PYLIBCZI_INDEXMAP_H
