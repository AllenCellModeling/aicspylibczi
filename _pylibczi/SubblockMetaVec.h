//
// Created by Jamie Sherman on 2019-11-13.
//

#ifndef _PYLIBCZI_SUBBLOCKMETAVEC_H
#define _PYLIBCZI_SUBBLOCKMETAVEC_H

#include <vector>
#include "inc_libCZI.h"
#include "Image.h"
#include "SubblockSorter.h"

namespace pylibczi {

  class SubblockString : public SubblockSorter {
      std::string m_string;
  public:
      SubblockString(libCZI::CDimCoordinate plane_, int index_m_, const std::string* str_p_)
      : SubblockSorter(std::move(plane_), index_m_), m_string(*str_p_) {}

      std::string getString() const { return m_string; }
  };

  class SubblockMetaVec: public std::vector< SubblockString > {
      bool m_isMosaic = false;
  public:
      SubblockMetaVec(): std::vector< SubblockString >() {}

      void setMosaic(bool mosaic_){ m_isMosaic = mosaic_; }

      void sort(){
          std::sort(begin(), end(), [](SubblockString& a_, SubblockString& b_)->bool{
              return a_ < b_;
          });
      }

      std::vector<std::pair<char, int> >
      getShape()
      {
          std::vector<std::vector<std::pair<char, int> > > validIndexes;
          for (auto& image : *this) {
              validIndexes.push_back(image.getValidIndexes(m_isMosaic)); // only add M if it's a mosaic file
          }

          std::vector<std::pair<char, int> > charSizes;
          std::set<int> condensed;
          for (int i = 0; !validIndexes.empty() && i<validIndexes.front().size(); i++) {
              char c;
              for (const auto& vi : validIndexes) {
                  c = vi[i].first;
                  condensed.insert(vi[i].second);
              }
              charSizes.emplace_back(c, condensed.size());
              condensed.clear();
          }
          return charSizes;
      }
  };
}

#endif //_PYLIBCZI_SUBBLOCKMETAVEC_H
