#include "exceptions.h"

namespace pylibczi {

  const std::map<libCZI::PixelType, const std::string> PixelTypeException::s_byName{
      {libCZI::PixelType::Invalid, "Invalid"},
      {libCZI::PixelType::Gray8, "Gray8"},
      {libCZI::PixelType::Gray16, "Gray16"},
      {libCZI::PixelType::Gray32Float, "Gray32Float"},
      {libCZI::PixelType::Bgr24, "Bgr24"},
      {libCZI::PixelType::Bgr48, "Bgr48"},
      {libCZI::PixelType::Bgr96Float, "Bgr96Float"},
      {libCZI::PixelType::Bgra32, "Bgra32"},
      {libCZI::PixelType::Gray64ComplexFloat, "Gray64ComplexFloat"},
      {libCZI::PixelType::Bgr192ComplexFloat, "Bgr192ComplexFloat"},
      {libCZI::PixelType::Gray32, "Gray32"},
      {libCZI::PixelType::Gray64Float, "Gray64Float"}
  };

}
