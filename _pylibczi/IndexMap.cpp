#include <algorithm>

#include "IndexMap.h"

namespace pylibczi {

  const std::vector<libCZI::DimensionIndex> IndexMap::s_sortOrder{
      libCZI::DimensionIndex::V,
      libCZI::DimensionIndex::H,
      libCZI::DimensionIndex::I,
      libCZI::DimensionIndex::R,
      libCZI::DimensionIndex::S,
      libCZI::DimensionIndex::T,
      libCZI::DimensionIndex::C,
      libCZI::DimensionIndex::Z
  };

  IndexMap::IndexMap(int index_, const libCZI::SubBlockInfo& info_)
      :m_subBlockIndex(index_), m_dims(), m_position(-1)
  {
      info_.coordinate.EnumValidDimensions([&](libCZI::DimensionIndex dimension_, int value_) {
          m_dims.emplace(dimension_, value_);
          return true;
      });
      m_indexM = info_.mIndex;
  }

  bool
  IndexMap::isMIndexValid() const
  {
      return m_indexM!=(std::numeric_limits<int>::min)();
  }

  bool
  IndexMap::operator<(const IndexMap& other_)
  {

      auto match = std::find_if(s_sortOrder.begin(), s_sortOrder.end(), [&](const libCZI::DimensionIndex& dim_) {

          auto matchDim = [dim_](const MapType::value_type& p_) -> bool {
              return (p_.first==dim_);
          };
          auto thisMatchIterator = std::find_if(m_dims.begin(), m_dims.end(), matchDim);
          auto otherMatchIterator = std::find_if(other_.m_dims.begin(), other_.m_dims.end(), matchDim);
          if (thisMatchIterator==m_dims.end() || otherMatchIterator==other_.m_dims.end())
              return false;
          return (thisMatchIterator->second<otherMatchIterator->second);
      });

      if (match==s_sortOrder.end() && isMIndexValid() && other_.isMIndexValid()) {
          return (m_indexM<other_.m_indexM);
      }
      return false;
  }

}
