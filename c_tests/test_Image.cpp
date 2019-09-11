//
// Created by Jamie Sherman on 2019-09-10.
//

#include "catch.hpp"
#include "Image.h"
#include "Reader.h"
#include "exceptions.h"

using namespace pylibczi;

TEST_CASE("imagefactory_pixeltype", "[ImageFactory_PixelType]"){
    REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Gray8) == 1);
    REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Gray16) == 2);
    REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Bgr24) == 1);
    REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Bgr48) == 2);
    REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Gray32Float) == 4);
    REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Bgr96Float) == 4);
}

TEST_CASE("imagefactory_nofchannels", "[ImageFactory_NofChannels]"){
    REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Gray8) == 1);
    REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Gray16) == 1);
    REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Bgr24) == 3);
    REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Bgr48) == 3);
    REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Gray32Float) == 1);
    REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Bgr96Float) == 3);
}

class CziImageCreator{
  std::unique_ptr<Reader> m_czi;
public:
  CziImageCreator(): m_czi(new Reader( std::fopen("resources/s_1_t_1_c_1_z_1.czi", "rb" ))){}
  std::shared_ptr<ImageBC> get() {
      auto c_dims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B,0 },
                                            { libCZI::DimensionIndex::C,0 } };
      auto imvec = m_czi->read_selected(c_dims);
      return imvec->front();
  }
};

TEST_CASE_METHOD(CziImageCreator, "test_image_cast", "[Image_Cast]"){
    std::shared_ptr<ImageBC> img = get();
    REQUIRE(img.get()->is_type_match<uint16_t>());
    REQUIRE(!(img->is_type_match<uint8_t>()));
    REQUIRE(!(img->is_type_match<float>()));
}

TEST_CASE_METHOD(CziImageCreator, "test_image_throw", "[Image_Cast_Throw]"){
    std::shared_ptr<ImageBC> img = get();
    REQUIRE_THROWS_AS(ImageFactory::get_derived<uint8_t>(img), PixelTypeException);
}

TEST_CASE_METHOD(CziImageCreator, "test_image_nothrow", "[Image_Cast_Nothrow]"){
    std::shared_ptr<ImageBC> img = get();
    REQUIRE_NOTHROW( ImageFactory::get_derived<uint16_t>(img) );
}
