//
// Created by Jamie Sherman on 7/22/20.
//

#ifndef _AICSPYLIBCZI_IMAGESCONTAINER_H
#define _AICSPYLIBCZI_IMAGESCONTAINER_H

#include <memory>

#include "Image.h"
#include "Reader.h"

namespace pylibczi {

  template<typename T> class ImagesContainer;

  class ImagesContainerBase {
  public:
      using Shape = std::vector<std::pair<char, size_t> >;
      using ImagesContainerBasePtr = std::unique_ptr<ImagesContainerBase>;

      static ImagesContainerBasePtr getTypedAsBase(libCZI::PixelType& pixel_type_, size_t pixels_in_all_images_);

      template<typename T>
      ImagesContainer<T>* getBaseAsTyped(void)
      {
          return static_cast< ImagesContainer<T>*>(this);
      }

  private:
      ImageVector m_images;
      Shape m_shape;
  };

  template<typename T>
  class ImagesContainer: public ImagesContainerBase {
  private:
      std::unique_ptr<T> m_uniquePtr;
  public:
      ImagesContainer(libCZI::PixelType pixel_type_, size_t pixels_in_all_images_)
          :m_uniquePtr(new T[pixels_in_all_images_]) { }

      T* getPointerAtIndex(size_t position_ = 0)
      {
          return m_uniquePtr.get()+position_;
      }

  };

  ImagesContainerBase::ImagesContainerBasePtr
  ImagesContainerBase::getTypedAsBase(libCZI::PixelType& pixel_type_, size_t pixels_in_all_images_)
  {
      ImagesContainerBasePtr imageMemory;
      switch (pixel_type_) {
      case libCZI::PixelType::Gray8:imageMemory = std::make_unique<ImagesContainer<uint8_t> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Gray16:imageMemory = std::make_unique<ImagesContainer<uint16_t> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Gray32Float:imageMemory = std::make_unique<ImagesContainer<float> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgr24:imageMemory = std::make_unique<ImagesContainer<uint8_t> >(pixel_type_, 3*pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgr48:imageMemory = std::make_unique<ImagesContainer<uint16_t> >(pixel_type_, 3*pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgr96Float:imageMemory = std::make_unique<ImagesContainer<float> >(pixel_type_, 3*pixels_in_all_images_);
          break;
      }
      return imageMemory;
  }

}
#endif //_AICSPYLIBCZI_IMAGESCONTAINER_H
