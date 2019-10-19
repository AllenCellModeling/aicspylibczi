#include "ImageFactory.h"
#include "TypedImage.h"

namespace pylibczi {

  ImageFactory::CtorMap ImageFactory::s_pixelToImage{
      {PixelType::Gray8,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int mIndex_) {
           return std::shared_ptr<TypedImage<uint8_t>>(
               new TypedImage<uint8_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, mIndex_));
       }},
      {PixelType::Bgr24,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int mIndex_) {
           return std::shared_ptr<TypedImage<uint8_t>>(
               new TypedImage<uint8_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, mIndex_));
       }},
      {PixelType::Gray16,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int mIndex_) {
           return std::shared_ptr<TypedImage<uint16_t>>(
               new TypedImage<uint16_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, mIndex_));
       }},
      {PixelType::Bgr48,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int mIndex_) {
           return std::shared_ptr<TypedImage<uint16_t>>(
               new TypedImage<uint16_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, mIndex_));
       }},
      {PixelType::Gray32Float,
       [](std::vector<size_t> shape_, PixelType pixel_type__, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int mIndex_) {
           return std::shared_ptr<TypedImage<float>>(
               new TypedImage<float>(std::move(shape_), pixel_type__, plane_coordinate_, box_, mIndex_));
       }},
      {PixelType::Bgr96Float,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int mIndex_) {
           return std::shared_ptr<TypedImage<float>>(
               new TypedImage<float>(std::move(shape_), pixel_type_, plane_coordinate_, box_, mIndex_));
       }}
  };

  size_t
  ImageFactory::sizeOfPixelType(PixelType pixel_type_)
  {
      switch (pixel_type_) {
      case PixelType::Gray8:
      case PixelType::Bgr24:return sizeof(uint8_t);
      case PixelType::Gray16:
      case PixelType::Bgr48:return sizeof(uint16_t);
      case PixelType::Gray32Float:
      case PixelType::Bgr96Float:return sizeof(float);
      default:throw PixelTypeException(pixel_type_, "Pixel Type unsupported by libCZI.");
      }
  }

  size_t
  ImageFactory::numberOfChannels(libCZI::PixelType pixel_type_)
  {
      using PT = libCZI::PixelType;
      switch (pixel_type_) {
      case PT::Gray8:
      case PT::Gray16:
      case PT::Gray32Float:return 1;
      case PT::Bgr24:
      case PT::Bgr48:
      case PT::Bgr96Float:return 3;
      default:throw PixelTypeException(pixel_type_, "Pixel Type unsupported by libCZI.");
      }

  };

  std::shared_ptr<Image> ImageFactory::constructImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_,
      const libCZI::CDimCoordinate* plane_coordinate_,
      libCZI::IntRect box_,
      int mIndex_)
  {
      libCZI::IntSize size = bitmap_ptr_->GetSize();
      libCZI::PixelType pixelType = bitmap_ptr_->GetPixelType();

      std::vector<size_t> shape;
      size_t channels = numberOfChannels(pixelType);
      if (channels==3)
          shape.emplace_back(3);
      shape.emplace_back(size.h);
      shape.emplace_back(size.w);

      std::shared_ptr<Image> image = s_pixelToImage[pixelType](shape, pixelType, plane_coordinate_, box_, mIndex_);
      if (image==nullptr)
          throw std::bad_alloc();
      image->loadImage(bitmap_ptr_, channels);

      return std::shared_ptr<Image>(image);
  }
}
