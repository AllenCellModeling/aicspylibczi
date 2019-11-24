#ifndef _PYLIBCZI_IMAGEFACTORY_H
#define _PYLIBCZI_IMAGEFACTORY_H

#include "Image.h"
#include "TypedImage.h"
#include "exceptions.h"

namespace pylibczi {

  class ImageFactory {
      using PixelType = libCZI::PixelType;
      using CtorMap = std::map<libCZI::PixelType,
                               std::function<std::shared_ptr<Image>(
                                   std::vector<size_t>, libCZI::PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_,
                                   libCZI::IntRect box_, int mIndex_)
                               > >;

      static CtorMap s_pixelToImage;

  public:
      static size_t sizeOfPixelType(PixelType pixel_type_);

      static size_t numberOfChannels(PixelType pixel_type_);

      template<typename T>
      static std::shared_ptr< TypedImage<T> >
      getDerived(std::shared_ptr<Image> image_ptr_)
      {
          if (!image_ptr_->isTypeMatch<T>())
              throw PixelTypeException(image_ptr_->pixelType(), "TypedImage PixelType doesn't match requested memory type.");
          return std::dynamic_pointer_cast<TypedImage<T>>(image_ptr_);
      }

      static std::shared_ptr<Image>
      constructImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_, const libCZI::CDimCoordinate* plane_coordinate_,
          libCZI::IntRect box_,
          int index_m_);
  };
}

#endif //_PYLIBCZI_IMAGEFACTORY_H
