#include "ImageFactory.h"
#include "TypedImage.h"

namespace pylibczi {
  ImageFactory::ConstrMap ImageFactory::s_pixelToImage{
          {PT::Gray8, [](V_ST shp, PT pt, LCD* dims, IR ir, int m) {
              return std::shared_ptr<TypedImage<uint8_t>>(new TypedImage<uint8_t>(std::move(shp), pt, dims, ir, m));
          }},
          {PT::Bgr24, [](V_ST shp, PT pt, LCD* dims, IR ir, int m) {
              return std::shared_ptr<TypedImage<uint8_t>>(new TypedImage<uint8_t>(std::move(shp), pt, dims, ir, m));
          }},
          {PT::Gray16, [](V_ST shp, PT pt, LCD* dims, IR ir, int m) {
              return std::shared_ptr<TypedImage<uint16_t>>(new TypedImage<uint16_t>(std::move(shp), pt, dims, ir, m));
          }},
          {PT::Bgr48, [](V_ST shp, PT pt, LCD* dims, IR ir, int m) {
              return std::shared_ptr<TypedImage<uint16_t>>(new TypedImage<uint16_t>(std::move(shp), pt, dims, ir, m));
          }},
          {PT::Gray32Float, [](V_ST shp, PT pt, LCD* dims, IR ir, int m) {
              return std::shared_ptr<TypedImage<float>>(new TypedImage<float>(std::move(shp), pt, dims, ir, m));
          }},
          {PT::Bgr96Float, [](V_ST shp, PT pt, LCD* dims, IR ir, int m) {
              return std::shared_ptr<TypedImage<float>>(new TypedImage<float>(std::move(shp), pt, dims, ir, m));
          }}
  };

  size_t
  ImageFactory::size_of_pixel_type(PT pixel_type)
  {
      switch (pixel_type) {
      case PT::Gray8:
      case PT::Bgr24:return sizeof(uint8_t);
      case PT::Gray16:
      case PT::Bgr48:return sizeof(uint16_t);
      case PT::Gray32Float:
      case PT::Bgr96Float:return sizeof(float);
      default:throw PixelTypeException(pixel_type, "Pixel Type unsupported by libCZI.");
      }
  }

  size_t
  ImageFactory::n_of_channels(libCZI::PixelType pixel_type)
  {
      using PT = libCZI::PixelType;
      switch (pixel_type) {
      case PT::Gray8:
      case PT::Gray16:
      case PT::Gray32Float:return 1;
      case PT::Bgr24:
      case PT::Bgr48:
      case PT::Bgr96Float:return 3;
      default:throw PixelTypeException(pixel_type, "Pixel Type unsupported by libCZI.");
      }

  };

  std::shared_ptr<ImageBC> ImageFactory::construct_image(const std::shared_ptr<libCZI::IBitmapData>& pBitmap,
          const libCZI::CDimCoordinate* cdims,
          libCZI::IntRect box,
          int mIndex)
  {
      libCZI::IntSize size = pBitmap->GetSize();
      libCZI::PixelType pt = pBitmap->GetPixelType();

      std::vector<size_t> shp;
      size_t channels = n_of_channels(pt);
      if (channels==3)
          shp.emplace_back(3);
      shp.emplace_back(size.h);
      shp.emplace_back(size.w);

      std::shared_ptr<ImageBC> img = s_pixelToImage[pt](shp, pt, cdims, box, mIndex);
      if (img==nullptr)
          throw std::bad_alloc();
      img->load_image(pBitmap, channels);

      return std::shared_ptr<ImageBC>(img);
  }
}
