//
// Created by Jamie Sherman on 7/22/20.
//

#ifndef _AICSPYLIBCZI_IMAGESCONTAINER_H
#define _AICSPYLIBCZI_IMAGESCONTAINER_H

#include <memory>

#include "Image.h"

namespace pylibczi {

  template<typename T> class ImagesContainer;

  class ImagesContainerBase {
  public:
      using Shape = std::vector<std::pair<char, size_t> >;
      using ImagesContainerBasePtr = std::unique_ptr<ImagesContainerBase>;

  private:
      ImageVector m_images;
      Shape m_shape;
      libCZI::PixelType m_cziPixelType;

  public:
      static ImagesContainerBasePtr getTypedAsBase(libCZI::PixelType& pixel_type_, size_t pixels_in_all_images_);

      template<typename T>
      ImagesContainer<T>* getBaseAsTyped(void)
      {   // this has to be static_cast because of the templating and the polymorphism
          return dynamic_cast< ImagesContainer<T>*>(this);
      }

      virtual ~ImagesContainerBase() { }

      void addImage(std::shared_ptr<Image> img_) { m_images.push_back(img_); }

      size_t numberOfImages(void) { return m_images.size(); }

      ImageVector& images(void) { return m_images; }
      Shape& shape(void)
      {
          if (m_shape.empty() && m_images.size()!=0)
              m_shape = m_images.getShape();
          return m_shape;
      }

      void setCziFilePixelType(libCZI::PixelType pixel_type_)
      {
          m_cziPixelType = pixel_type_;
      }

      libCZI::PixelType pixelType(void)
      {
          return m_cziPixelType;
      }
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

      T* releaseMemory(void) { return m_uniquePtr.release(); }
  };

  inline
  ImagesContainerBase::ImagesContainerBasePtr
  ImagesContainerBase::getTypedAsBase(libCZI::PixelType& pixel_type_, size_t pixels_in_all_images_)
  {
      ImagesContainerBasePtr imageMemory;
      switch (pixel_type_) {
      case libCZI::PixelType::Gray8:imageMemory = std::make_unique<ImagesContainer<uint8_t> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Gray16:imageMemory = std::make_unique<ImagesContainer<uint16_t> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Gray32: imageMemory = std::make_unique<ImagesContainer<uint32_t> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Gray32Float:imageMemory = std::make_unique<ImagesContainer<float> >(pixel_type_, pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgr24:
          imageMemory = std::make_unique<ImagesContainer<uint8_t> >(libCZI::PixelType::Gray8, 3*pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgr48:
          imageMemory = std::make_unique<ImagesContainer<uint16_t> >(libCZI::PixelType::Gray16, 3*pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgr96Float:
          imageMemory = std::make_unique<ImagesContainer<float> >(libCZI::PixelType::Gray32Float, 3*pixels_in_all_images_);
          break;
      case libCZI::PixelType::Bgra32:
      case libCZI::PixelType::Gray64Float:
      case libCZI::PixelType::Gray64ComplexFloat:
      case libCZI::PixelType::Bgr192ComplexFloat:
      case libCZI::PixelType::Invalid:throw PixelTypeException(pixel_type_, "unsupported pixel type.");
      }
      imageMemory->setCziFilePixelType(pixel_type_); // make sure imageMemory has the original pixel type
      return imageMemory;
  }

}
#endif //_AICSPYLIBCZI_IMAGESCONTAINER_H
