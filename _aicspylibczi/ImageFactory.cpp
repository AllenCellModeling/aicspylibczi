#include "ImageFactory.h"
#include "TypedImage.h"

namespace pylibczi {

  ImageFactory::CtorMap ImageFactory::s_pixelToImage{
      {PixelType::Gray8,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int index_m_) {
           return std::shared_ptr<TypedImage<uint8_t>>(
               new TypedImage<uint8_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, index_m_));
       }},
      {PixelType::Bgr24,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int index_m_) {
           return std::shared_ptr<TypedImage<uint8_t>>(
               new TypedImage<uint8_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, index_m_));
       }},
      {PixelType::Gray16,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int index_m_) {
           return std::shared_ptr<TypedImage<uint16_t>>(
               new TypedImage<uint16_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, index_m_));
       }},
      {PixelType::Bgr48,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int index_m_) {
           return std::shared_ptr<TypedImage<uint16_t>>(
               new TypedImage<uint16_t>(std::move(shape_), pixel_type_, plane_coordinate_, box_, index_m_));
       }},
      {PixelType::Gray32Float,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int index_m_) {
           return std::shared_ptr<TypedImage<float>>(
               new TypedImage<float>(std::move(shape_), pixel_type_, plane_coordinate_, box_, index_m_));
       }},
      {PixelType::Bgr96Float,
       [](std::vector<size_t> shape_, PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinate_, libCZI::IntRect box_,
           int index_m_) {
           return std::shared_ptr<TypedImage<float>>(
               new TypedImage<float>(std::move(shape_), pixel_type_, plane_coordinate_, box_, index_m_));
       }}
  };

  ImageFactory::SplitCtorMap ImageFactory::s_pixelToSplit{
      {PixelType::Bgr24,
       [](std::shared_ptr<Image> img_, int channel_) {
           return std::shared_ptr<TypedImage<uint8_t>>(
               new TypedImage<uint8_t>(ImageFactory::getDerived< uint8_t >(img_), libCZI::PixelType::Gray8, channel_));
       }},
      {PixelType::Bgr48,
       [](std::shared_ptr<Image> img_, int channel_) {
           return std::shared_ptr<TypedImage<uint16_t>>(
               new TypedImage<uint16_t>(ImageFactory::getDerived< uint16_t >(img_), libCZI::PixelType::Gray16, channel_));
       }},
      {PixelType::Bgr96Float,
       [](std::shared_ptr<Image> img_, int channel_) {
           return std::shared_ptr<TypedImage<float>>(
               new TypedImage<float>(ImageFactory::getDerived< float >(img_), libCZI::PixelType::Gray32Float, channel_));
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
      switch (pixel_type_) {
      case PixelType::Gray8:
      case PixelType::Gray16:
      case PixelType::Gray32Float:return 1;
      case PixelType::Bgr24:
      case PixelType::Bgr48:
      case PixelType::Bgr96Float:return 3;
      default:throw PixelTypeException(pixel_type_, "Pixel Type unsupported by libCZI.");
      }
  }

  std::shared_ptr<Image>
  ImageFactory::constructImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_,
      const libCZI::CDimCoordinate* plane_coordinate_,
      libCZI::IntRect box_,
      int index_m_)
  {
      libCZI::IntSize size = bitmap_ptr_->GetSize();
      PixelType pixelType = bitmap_ptr_->GetPixelType();

      std::vector<size_t> shape;
      size_t channels = numberOfChannels(pixelType);
      if (channels==3)
          shape.emplace_back(3);
      shape.emplace_back(size.h);
      shape.emplace_back(size.w);

      std::shared_ptr<Image> image = s_pixelToImage[pixelType](shape, pixelType, plane_coordinate_, box_, index_m_);
      if (image==nullptr)
          throw std::bad_alloc();
      image->loadImage(bitmap_ptr_, channels);

      return std::shared_ptr<Image>(image);
  }

  Image::ImVec ImageFactory::splitToChannels(std::shared_ptr<Image> img_in_)
  {
      Image::ImVec ivec;
      auto shape = img_in_->shape();
      if (shape.size() != 3)
          throw ImageSplitChannelException("TypedImage  only has 2 dimensions. No channels to split.", 0);
      int cStart = 0;
      // TODO figure out if C can have a nonzero value for a BGR image
      if ( img_in_->coordinatePtr()->TryGetPosition(libCZI::DimensionIndex::C, &cStart) && cStart != 0)
          throw ImageSplitChannelException("attempting to split channels", cStart);
      for (int i = 0; i<shape[0]; i++) {
          // TODO should I change the pixel type from a BGRx to a Grayx/3
          libCZI::PixelType pt = img_in_->pixelType();
          if(pt == libCZI::PixelType::Invalid){
              throw ImageSplitChannelException("Only PixelTypes Bgr24, Bgr48, and Bgr96Float can be split! "
                                               "You have attempted to split an invalid PixelType", cStart);
          }
          ivec.emplace_back(s_pixelToSplit[pt]( img_in_, i));
      }
      return ivec;
  }
}
