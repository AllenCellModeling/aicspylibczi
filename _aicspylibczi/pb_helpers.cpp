#include <memory>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <set>
#include <vector>

#include "Reader.h"
#include "exceptions.h"
#include "pb_helpers.h"

namespace pb_helpers {

std::vector<std::pair<char, size_t>>
getAndFixShape(pylibczi::ImagesContainerBase* bptr_)
{
  auto images = bptr_->images();
  images.sort();
  auto charSizes = images.getShape();
  return charSizes;
}

py::array
packArray(pylibczi::ImagesContainerBase::ImagesContainerBasePtr& base_ptr_)
{
  pylibczi::ImagesContainerBase* icBase = base_ptr_.release();
  std::vector<std::pair<char, size_t>> charSizes;

  charSizes = getAndFixShape(icBase);
  py::array* arr;

  switch (icBase->pixelType()) {
    case libCZI::PixelType::Gray8:
    case libCZI::PixelType::Bgr24:
      arr = memoryToNpArray<uint8_t>(icBase, charSizes);
      break;
    case libCZI::PixelType::Gray16:
    case libCZI::PixelType::Bgr48:
      arr = memoryToNpArray<uint16_t>(icBase, charSizes);
      break;
    case libCZI::PixelType::Gray32:
      arr = memoryToNpArray<uint32_t>(icBase, charSizes);
      break;
    case libCZI::PixelType::Gray32Float:
    case libCZI::PixelType::Bgr96Float:
      arr = memoryToNpArray<float>(icBase, charSizes);
      break;
    default:
      throw pylibczi::PixelTypeException(icBase->pixelType(), "Unsupported pixel type in helper function.");
      // It is highly unlikely the this throw would ever be reached but it's here for completeness. If an unsupported
      // pixel type were encountered it would throw much eariler in the code.
  }
  return *arr;
}

py::list*
packStringArray(pylibczi::SubblockMetaVec& metadata_)
{
  metadata_.sort();
  auto charSizes = metadata_.getShape();
  auto mylist = new py::list();
  try {
    for (const auto& x : metadata_) {
      mylist->append(py::make_tuple(x.getDimsAsChars(), py::cast(x.getString().c_str())));
    }
  } catch (exception& e) {
    std::cout << e.what() << std::endl;
  }
  return mylist;
}

}
