#ifndef _PYLIBCZI_SUBBLOCKSORTER_H
#define _PYLIBCZI_SUBBLOCKSORTER_H

#include <utility>
#include <vector>

#include "inc_libCZI.h"

namespace pylibczi{

  class SubblockSorter{
  protected:
    libCZI::CDimCoordinate m_planeCoordinate;
    int m_indexM;
  public:
      SubblockSorter(libCZI::CDimCoordinate  plane_, int index_m_)
      : m_planeCoordinate(std::move(plane_)), m_indexM(index_m_) {}

      static std::vector<std::pair<char, int> > getValidIndexes(const libCZI::CDimCoordinate& planecoord_, int index_m_, bool is_mosaic_ = false)
      {
          using CziDi = libCZI::DimensionIndex;
          std::initializer_list<CziDi> sortOrder;
          sortOrder = {CziDi::V, CziDi::H, CziDi::I, CziDi::S, CziDi::R, CziDi::T, CziDi::C, CziDi::Z};
          std::vector<std::pair<char, int> > ans;
          for (auto di : sortOrder) {
              int value;
              if (planecoord_.TryGetPosition(di, &value)) ans.emplace_back(libCZI::Utils::DimensionToChar(di), value);
          }
          if (is_mosaic_) ans.emplace_back('M', index_m_);
          return ans;
      }

      std::vector<std::pair<char, int> > getValidIndexes(bool is_mosaic_ = false){
          return SubblockSorter::getValidIndexes(m_planeCoordinate, m_indexM, is_mosaic_);
      }

      bool operator<(const SubblockSorter& other_){
          return SubblockSorter::aLessThanB(m_planeCoordinate, m_indexM, other_.m_planeCoordinate, other_.m_indexM);
      }

      static bool aLessThanB(const libCZI::CDimCoordinate &a_, const libCZI::CDimCoordinate &b_){
          using CziDi = libCZI::DimensionIndex;
          std::initializer_list<CziDi> allDi;
          allDi = {CziDi::V, CziDi::H, CziDi::I, CziDi::S, CziDi::R, CziDi::T, CziDi::C, CziDi::Z};
          for(auto di : allDi){
              int aValue, bValue;
              if(a_.TryGetPosition(di, &aValue) && b_.TryGetPosition(di, &bValue) &&  aValue != bValue)
                  return aValue < bValue;
          }
          return false;
      }

      static bool aLessThanB(const libCZI::CDimCoordinate &a_, const int a_index_m_, const libCZI::CDimCoordinate &b_, const int b_index_m_){
          if( !aLessThanB(a_, b_) && !aLessThanB(b_, a_) )
              return a_index_m_ < b_index_m_;
          return aLessThanB(a_, b_);
      }
  };
}

#endif //_PYLIBCZI_SUBBLOCKSORTER_H
