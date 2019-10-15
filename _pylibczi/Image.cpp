//
// Created by Jamie Sherman on 2019-08-28.
//

#include <algorithm>
#include <numeric>
#include <typeinfo>
#include <utility>
#include <iostream>
#include <cstdint>

#include "Image.h"
#include "Iterator.h"
#include "exceptions.h"
#include "helper_algorithms.h"

namespace pylibczi {

  std::map<libCZI::PixelType, std::string>  ImageBC::s_pixelToTypeName{
          {libCZI::PixelType::Gray8, typeid(uint8_t).name()},        // 8-bit grayscale
          {libCZI::PixelType::Gray16, typeid(uint16_t).name()},       // 16-bit grayscale
          {libCZI::PixelType::Gray32Float, typeid(float).name()},          // 4-byte float
          {libCZI::PixelType::Bgr24, typeid(uint8_t).name()},        // 8-bit triples (order B, G, R).
          {libCZI::PixelType::Bgr48, typeid(uint16_t).name()},       // 16-bit triples (order B, G, R).
          {libCZI::PixelType::Bgr96Float, typeid(float).name()},          // 4-byte triples (order B, G, R).
          {libCZI::PixelType::Bgra32, typeid(nullptr).name()},    // unsupported by libCZI
          {libCZI::PixelType::Gray64ComplexFloat, typeid(nullptr).name()},    // unsupported by libCZI
          {libCZI::PixelType::Bgr192ComplexFloat, typeid(nullptr).name()},    // unsupported by libCZI
          {libCZI::PixelType::Gray32, typeid(nullptr).name()},    // unsupported by libCZI
          {libCZI::PixelType::Gray64Float, typeid(nullptr).name()}     // unsupported by libCZI
  };

  std::vector<std::pair<char, int> > ImageBC::get_valid_indexs(bool isMosaic)
  {
      using CZI_DI = libCZI::DimensionIndex;
      std::vector<CZI_DI> sort_order{CZI_DI::S, CZI_DI::T, CZI_DI::C, CZI_DI::Z};
      std::vector<std::pair<char, int> > ans;
      for (auto di : sort_order) {
          int value;
          if (m_planeCoordinates.TryGetPosition(di, &value)) ans.emplace_back(libCZI::Utils::DimensionToChar(di), value);
      }
      if (isMosaic) ans.emplace_back('M', m_mIndex);
      return ans;
  }

  bool ImageBC::operator<(ImageBC& other)
  {
      using CZI_DI = libCZI::DimensionIndex;
      std::vector<CZI_DI> sort_order{CZI_DI::S, CZI_DI::T, CZI_DI::C, CZI_DI::Z};
      for (auto di : sort_order) {
          int di_value, other_value;
          if (m_planeCoordinates.TryGetPosition(di, &di_value) && other.m_planeCoordinates.TryGetPosition(di, &other_value) && di_value!=other_value)
              return (di_value<other_value);
      }
      return m_mIndex<other.m_mIndex;
  }

  size_t ImageBC::calculate_idx(const std::vector<size_t>& idxs)
  {
      if (idxs.size()!=m_shape.size())
          throw ImageAccessUnderspecifiedException(idxs.size(), m_shape.size(), "Sizes must match");
      size_t running_product = 1;
      std::vector<size_t> weights(1, 1);
      std::for_each(m_shape.rbegin(), m_shape.rend()-1, [&weights, &running_product](const size_t len) {
          running_product *= len;
          weights.emplace_back(running_product);
      });
      std::vector<size_t> prod(m_shape.size(), 0);
      std::transform(idxs.begin(), idxs.end(), weights.begin(), prod.begin(), [](size_t a, size_t b) -> size_t {
          return a*b;
      });
      size_t idx = std::accumulate(prod.begin(), prod.end(), size_t(0));
      return idx;
  }


}
