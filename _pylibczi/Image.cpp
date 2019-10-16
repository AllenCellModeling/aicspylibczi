#include <algorithm>
#include <numeric>
#include <utility>
#include <cstdint>

#include "Image.h"
#include "exceptions.h"

namespace pylibczi {

  std::map<libCZI::PixelType, std::string>  Image::s_pixelToTypeName{
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

  std::vector<std::pair<char, int> > Image::getValidIndexes(bool is_mosaic_)
  {
      using CziDi = libCZI::DimensionIndex;
      std::vector<CziDi> sortOrder{CziDi::S, CziDi::T, CziDi::C, CziDi::Z};
      std::vector<std::pair<char, int> > ans;
      for (auto di : sortOrder) {
          int value;
          if (m_planeCoordinates.TryGetPosition(di, &value)) ans.emplace_back(libCZI::Utils::DimensionToChar(di), value);
      }
      if (is_mosaic_) ans.emplace_back('M', m_mIndex);
      return ans;
  }

  bool Image::operator<(Image& other_)
  {
      using CziDi = libCZI::DimensionIndex;
      std::vector<CziDi> sortOrder{CziDi::S, CziDi::T, CziDi::C, CziDi::Z};
      for (auto di : sortOrder) {
          int diValue, otherValue;
          if (m_planeCoordinates.TryGetPosition(di, &diValue) && other_.m_planeCoordinates.TryGetPosition(di, &otherValue)
              && diValue!=otherValue)
              return (diValue<otherValue);
      }
      return m_mIndex<other_.m_mIndex;
  }

  size_t Image::calculateIdx(const std::vector<size_t>& indexes_)
  {
      if (indexes_.size()!=m_shape.size())
          throw ImageAccessUnderspecifiedException(indexes_.size(), m_shape.size(), "Sizes must match");
      size_t runningProduct = 1;
      std::vector<size_t> weights(1, 1);
      std::for_each(m_shape.rbegin(), m_shape.rend()-1, [&weights, &runningProduct](const size_t length_) {
          runningProduct *= length_;
          weights.emplace_back(runningProduct);
      });
      std::vector<size_t> prod(m_shape.size(), 0);
      std::transform(indexes_.begin(), indexes_.end(), weights.begin(), prod.begin(), [](size_t a_, size_t b_) -> size_t {
          return a_*b_;
      });
      size_t idx = std::accumulate(prod.begin(), prod.end(), size_t(0));
      return idx;
  }

}
