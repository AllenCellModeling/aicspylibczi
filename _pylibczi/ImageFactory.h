#ifndef _PYLIBCZI_IMAGEFACTORY_H
#define _PYLIBCZI_IMAGEFACTORY_H

#include "Image.h"
#include "exceptions.h"

namespace pylibczi {

  template<typename T>
  class TypedImage;

  class ImageFactory {
      using PT = libCZI::PixelType;
      using V_ST = std::vector<size_t>;
      using ConstrMap = std::map<libCZI::PixelType,
                                 std::function<std::shared_ptr<ImageBC>(
                                         std::vector<size_t>, libCZI::PixelType pt, const libCZI::CDimCoordinate* cdim,
                                         libCZI::IntRect ir, int mIndex)
                                 > >;

      using LCD = const libCZI::CDimCoordinate;
      using IR = libCZI::IntRect;

      static ConstrMap s_pixelToImage;

  public:
      static size_t size_of_pixel_type(PT pixel_type);

      static size_t n_of_channels(PT pixel_type);

      template<typename T>
      static std::shared_ptr<TypedImage<T> >
      get_derived(std::shared_ptr<ImageBC>
      ptr)
      {
          if (!ptr->is_type_match<T>())
              throw PixelTypeException(ptr->pixelType(), "TypedImage PixelType doesn't match requested memory type.");
          return std::dynamic_pointer_cast<TypedImage<T>>(ptr);
      }

      static std::shared_ptr<ImageBC>
      construct_image(const std::shared_ptr<libCZI::IBitmapData>& pBitmap, const libCZI::CDimCoordinate* cdims, libCZI::IntRect box,
              int mIndex);
  };
}

#endif //_PYLIBCZI_IMAGEFACTORY_H
