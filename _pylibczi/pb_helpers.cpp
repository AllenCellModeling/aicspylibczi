#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <set>

#include "pb_helpers.h"
#include "Image.h"
#include "Reader.h"
#include "exceptions.h"

namespace pb_helpers {

  py::array packArray(pylibczi::ImageVector& images_)
  {
      // assumptions: The array contains images of the same size and the array is contiguous.
      auto charSizes = pylibczi::Reader::getShape(images_, images_.isMosaic());
      unsigned long newSize = images_.front()->length()*images_.size();
      std::vector<ssize_t> shape(charSizes.size(), 0);
      std::transform(charSizes.begin(), charSizes.end(), shape.begin(), [](const std::pair<char, int>& a) {
          return a.second;
      });
      py::array* arrP = nullptr;
      switch (images_.front()->pixelType()) {
      case libCZI::PixelType::Gray8:
      case libCZI::PixelType::Bgr24: arrP = makeArray<uint8_t>(newSize, shape, images_);
          break;
      case libCZI::PixelType::Gray16:
      case libCZI::PixelType::Bgr48: arrP = makeArray<uint16_t>(newSize, shape, images_);
          break;
      case libCZI::PixelType::Gray32Float:
      case libCZI::PixelType::Bgr96Float: arrP = makeArray<float>(newSize, shape, images_);
          break;
      default: throw pylibczi::PixelTypeException(images_.front()->pixelType(), "Unsupported pixel type");
      }
      return *arrP;
  }

}
