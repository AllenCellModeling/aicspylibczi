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

  std::vector<std::pair<char, size_t> > getAndFixShape(pylibczi::ImagesContainerBase *bptr_){
      auto images = bptr_->images();
      images.sort();
      auto charSizes = images.getShape();
      if( bptr_->pixelType() == libCZI::PixelType::Bgr24 ||
          bptr_->pixelType() == libCZI::PixelType::Bgr48 ||
          bptr_->pixelType() == libCZI::PixelType::Bgr96Float ){
          auto ittr = find_if(charSizes.begin(), charSizes.end(), [](std::pair<char, size_t> &pr){
              return (pr.first == 'C');
          });
          if( ittr != charSizes.end()) ittr->second *=3;
          else {
              charSizes.push_back(std::pair<char, size_t>('C', 3));
              std::sort(charSizes.begin(), charSizes.end(), [&](std::pair<char, size_t> a_, std::pair<char, size_t> b_) {
                  return libCZI::Utils::CharToDimension(a_.first)>libCZI::Utils::CharToDimension(b_.first);
              });
          }
      }
      return charSizes;
  }
  
  py::array packArray(pylibczi::ImagesContainerBase::ImagesContainerBasePtr &base_ptr_)
  {
      pylibczi::ImagesContainerBase *icBase = base_ptr_.release();
      std::vector<std::pair<char, size_t> > charSizes;

      charSizes = getAndFixShape(icBase);
      py::array *arr;

      switch (icBase->pixelType()){
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
      }
      return *arr;
  }

  py::list* packStringArray(pylibczi::SubblockMetaVec& metadata_){
      metadata_.sort();
      auto charSizes = metadata_.getShape();
      auto mylist = new py::list();
      try {
          for (const auto& x : metadata_) {
              mylist->append(py::make_tuple( x.getDimsAsChars(), py::cast(x.getString().c_str()) ) );
          }
      }catch(exception &e){
          std::cout << e.what() << std::endl;
      }
      return mylist;
  }


}
