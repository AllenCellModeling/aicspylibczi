#ifndef _PYLIBCZI_IMAGEFACTORY_H
#define _PYLIBCZI_IMAGEFACTORY_H

#include <mutex>

#include "Image.h"
#include "ImagesContainer.h"
#include "TypedImage.h"
#include "exceptions.h"

namespace pylibczi {

class ImageFactory
{
  using PixelType = libCZI::PixelType;
  using CtorMap = std::map<libCZI::PixelType,
                           std::function<std::shared_ptr<Image>(std::vector<size_t>,
                                                                libCZI::PixelType pixel_type_,
                                                                const libCZI::CDimCoordinate* plane_coordinate_,
                                                                libCZI::IntRect box_,
                                                                ImagesContainerBase* bptr,
                                                                size_t img_index_,
                                                                int mIndex_)>>;
  using SplitCtorMap =
    std::map<libCZI::PixelType, std::function<std::shared_ptr<Image>(std::shared_ptr<Image> img_, int channel_)>>;

  static CtorMap s_pixelToImageConstructor;

  ImagesContainerBase::ImagesContainerBasePtr m_imgContainer;

public:
  ImageFactory(libCZI::PixelType pixel_type_, size_t pixels_in_all_images_)
    : m_imgContainer(ImagesContainerBase::getTypedAsBase(pixel_type_, pixels_in_all_images_))
  {}

  ImagesContainerBase::ImagesContainerBasePtr transferMemoryContainer(void)
  {
    return std::move(m_imgContainer); // this should empty m_imgContainer
  }

  char* mem_start(void) { return (char*)(m_imgContainer.get()); }

  ImageVector& images() { return m_imgContainer->images(); }

  size_t numberOfImages(void) { return m_imgContainer->numberOfImages(); }

  static size_t sizeOfPixelType(PixelType pixel_type_);

  static size_t numberOfSamples(PixelType pixel_type_);

  void setMosaic(bool val_) { m_imgContainer->images().setMosaic(val_); }

  template<typename T>
  static std::shared_ptr<TypedImage<T>> getDerived(std::shared_ptr<Image> image_ptr_)
  {
    if (!image_ptr_->isTypeMatch<T>())
      throw PixelTypeException(image_ptr_->pixelType(), "TypedImage PixelType doesn't match requested memory type.");
    return std::dynamic_pointer_cast<TypedImage<T>>(image_ptr_);
  }

  std::shared_ptr<Image> constructImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_,
                                        libCZI::IntSize size_,
                                        const libCZI::CDimCoordinate* plane_coordinate_,
                                        libCZI::IntRect box_,
                                        size_t mem_index_,
                                        int index_m_);

  vector<std::pair<char, size_t>> getFixedShape(void);
};
}

#endif //_PYLIBCZI_IMAGEFACTORY_H
