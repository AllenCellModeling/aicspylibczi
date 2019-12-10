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

std::map<libCZI::PixelType, libCZI::PixelType> Image::s_pixelSplitMap{
    {libCZI::PixelType::Gray8, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Gray16, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Gray32Float, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Bgr24, libCZI::PixelType::Gray8},
    {libCZI::PixelType::Bgr48, libCZI::PixelType::Gray16},
    {libCZI::PixelType::Bgr96Float, libCZI::PixelType::Gray32Float},
    {libCZI::PixelType::Bgra32, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Gray64ComplexFloat, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Bgr192ComplexFloat, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Gray32, libCZI::PixelType::Invalid},
    {libCZI::PixelType::Gray64Float, libCZI::PixelType::Invalid}
};





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
