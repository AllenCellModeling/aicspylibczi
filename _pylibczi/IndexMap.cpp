//
// Created by Jamie Sherman on 2019-08-17.
//

#include <cstdio>
#include <cstdlib>
#include "IndexMap.h"
#include <algorithm>

namespace pylibczi {

  const std::vector<libCZI::DimensionIndex> IndexMap::m_sortOrder = {
          libCZI::DimensionIndex::V,
          libCZI::DimensionIndex::H,
          libCZI::DimensionIndex::I,
          libCZI::DimensionIndex::R,
          libCZI::DimensionIndex::S,
          libCZI::DimensionIndex::T,
          libCZI::DimensionIndex::C,
          libCZI::DimensionIndex::Z
  };

  IndexMap::IndexMap(int idx, const libCZI::SubBlockInfo& info)
          :m_subblockIndex(idx), m_dims(), m_position(-1)
  {
      info.coordinate.EnumValidDimensions([&](libCZI::DimensionIndex dim, int value) {
          m_dims.emplace(dim, value);
          return true;
      });
      m_index = info.mIndex;
  }

  bool
  IndexMap::IsMIndexValid() const
  {
      return m_index!=(std::numeric_limits<int>::min)();
  }

  bool
  IndexMap::operator<(const IndexMap& b)
  {

      auto match = std::find_if(m_sortOrder.begin(), m_sortOrder.end(), [&](const libCZI::DimensionIndex& dim) {

          auto matchDim = [dim](const map_type::value_type& p) -> bool {
              return (p.first==dim);
          };
          auto m_itt = std::find_if(m_dims.begin(), m_dims.end(), matchDim);
          auto b_itt = std::find_if(b.m_dims.begin(), b.m_dims.end(), matchDim);
          if (m_itt==m_dims.end() || b_itt==b.m_dims.end())
              return false;
          return (m_itt->second<b_itt->second);
      });

      if (match==m_sortOrder.end() && IsMIndexValid() && b.IsMIndexValid()) {
          return (m_index<b.m_index);
      }
      return false;
  }

}
