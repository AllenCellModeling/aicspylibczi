#include "catch.hpp"
#include "inc_libCZI.h"

#include "ImageFactory.h"
#include "Reader.h"
#include "TargetRange.h"
#include "TypedImage.h"
#include "exceptions.h"
#include "helper_algorithms.h"

using namespace pylibczi;

TEST_CASE("imagefactory_pixeltype", "[ImageFactory_PixelType]")
{
  REQUIRE(ImageFactory::sizeOfPixelType(libCZI::PixelType::Gray8) == 1);
  REQUIRE(ImageFactory::sizeOfPixelType(libCZI::PixelType::Gray16) == 2);
  REQUIRE(ImageFactory::sizeOfPixelType(libCZI::PixelType::Bgr24) == 1);
  REQUIRE(ImageFactory::sizeOfPixelType(libCZI::PixelType::Bgr48) == 2);
  REQUIRE(ImageFactory::sizeOfPixelType(libCZI::PixelType::Gray32Float) == 4);
  REQUIRE(ImageFactory::sizeOfPixelType(libCZI::PixelType::Bgr96Float) == 4);
}

TEST_CASE("imagefactory_nofchannels", "[ImageFactory_NofChannels]")
{
  REQUIRE(ImageFactory::numberOfSamples(libCZI::PixelType::Gray8) == 1);
  REQUIRE(ImageFactory::numberOfSamples(libCZI::PixelType::Gray16) == 1);
  REQUIRE(ImageFactory::numberOfSamples(libCZI::PixelType::Bgr24) == 3);
  REQUIRE(ImageFactory::numberOfSamples(libCZI::PixelType::Bgr48) == 3);
  REQUIRE(ImageFactory::numberOfSamples(libCZI::PixelType::Gray32Float) == 1);
  REQUIRE(ImageFactory::numberOfSamples(libCZI::PixelType::Bgr96Float) == 3);
}

class CziImageCreator
{
  std::unique_ptr<Reader> m_czi;

public:
  CziImageCreator()
    : m_czi(new pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi"))
  {}
  std::shared_ptr<Image> get()
  {
    auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 }, { libCZI::DimensionIndex::C, 0 } };
    auto imgCont = m_czi->readSelected(cDims, -1, 1);
    auto imVec = imgCont.first->images();
    return imVec.front();
  }
};

TEST_CASE_METHOD(CziImageCreator, "test_image_cast", "[Image_Cast]")
{
  std::shared_ptr<Image> img = get();
  REQUIRE(img.get()->isTypeMatch<uint16_t>());
  REQUIRE(!(img->isTypeMatch<uint8_t>()));
  REQUIRE(!(img->isTypeMatch<float>()));
}

TEST_CASE_METHOD(CziImageCreator, "test_image_throw", "[Image_Cast_Throw]")
{
  std::shared_ptr<Image> img = get();
  REQUIRE_THROWS_AS(ImageFactory::getDerived<uint8_t>(img), PixelTypeException);
}

TEST_CASE_METHOD(CziImageCreator, "test_image_nothrow", "[Image_Cast_Nothrow]")
{
  std::shared_ptr<Image> img = get();
  REQUIRE_NOTHROW(ImageFactory::getDerived<uint16_t>(img));
}

TEST_CASE("test_image_accessors", "[Image_operator[]]")
{
  libCZI::CDimCoordinate cdim{ { libCZI::DimensionIndex::S, 1 }, { libCZI::DimensionIndex::C, 1 } };
  auto uMemPtr = std::unique_ptr<uint16_t>(new uint16_t[60]);
  TypedImage<uint16_t> img({ 3, 4, 5 }, libCZI::PixelType::Gray16, &cdim, { 0, 0, 5, 4 }, uMemPtr.get(), -1);
  uint16_t ip[60];
  for (int i = 0; i < 3 * 4 * 5; i++)
    ip[i] = i / 3 + 1;

  pylibczi::SourceRange<uint16_t> sourceRange(3, ip, ip + 60, 30, 5);
  pylibczi::TargetRange<uint16_t> targetRange(3, 5, 4, img.getRawPtr(), img.getRawPtr(60));

  pylibczi::SourceRange<uint16_t>::SourceChannelIterator beg = sourceRange.strideBegin(1);
  pylibczi::SourceRange<uint16_t>::SourceChannelIterator end = sourceRange.strideEnd(0);
  REQUIRE(beg == end);
  REQUIRE(sourceRange.strideBegin(2) == sourceRange.strideEnd(1));
  REQUIRE(sourceRange.strideBegin(3) == sourceRange.strideEnd(2));

  for (int i = 0; i < 4; i++) { // copy stride by stride as you would with an actual image
    pairedForEach(sourceRange.strideBegin(i),
                  sourceRange.strideEnd(i),
                  targetRange.strideBegin(i),
                  [](std::vector<uint16_t*> a, std::vector<uint16_t*> b) {
                    pairedForEach(a.begin(), a.end(), b.begin(), [](uint16_t* ai, uint16_t* bi) { *bi = *ai; });
                  });
  }
  int cnt = 0;
  for (size_t k = 0; k < 3; k++)
    for (size_t j = 0; j < 4; j++)
      for (size_t i = 0; i < 5; i++)
        REQUIRE(cnt++ == img.calculateIdx({ i, j, k }));

  cnt = 0;
  for (size_t k = 0; k < 3; k++)
    for (size_t j = 0; j < 4; j++)
      for (size_t i = 0; i < 5; i++)
        REQUIRE(img[{ i, j, k }] == *img.getRawPtr(cnt++));
}

TEST_CASE("test_image_accessors_2d", "[Image_operator[2d]]")
{
  libCZI::CDimCoordinate cdim{ { libCZI::DimensionIndex::S, 1 }, { libCZI::DimensionIndex::C, 1 } };
  auto uMemPtr = std::unique_ptr<uint16_t>(new uint16_t[20]);
  TypedImage<uint16_t> img({ 4, 5 }, libCZI::PixelType::Gray16, &cdim, { 0, 0, 5, 4 }, uMemPtr.get(), -1);
  uint16_t ip[20];
  for (int i = 0; i < 4 * 5; i++)
    ip[i] = i + 1;

  pylibczi::SourceRange<uint16_t> sourceRange(1, ip, ip + 20, 10, 5);
  pylibczi::TargetRange<uint16_t> targetRange(1, 5, 4, img.getRawPtr(), img.getRawPtr(20));

  pylibczi::SourceRange<uint16_t>::SourceChannelIterator beg = sourceRange.strideBegin(1);
  pylibczi::SourceRange<uint16_t>::SourceChannelIterator end = sourceRange.strideEnd(0);
  REQUIRE(beg == end);
  REQUIRE(sourceRange.strideBegin(2) == sourceRange.strideEnd(1));
  REQUIRE(sourceRange.strideBegin(3) == sourceRange.strideEnd(2));

  for (int i = 0; i < 4; i++) { // copy stride by stride as you would with an actual image
    pairedForEach(sourceRange.strideBegin(i),
                  sourceRange.strideEnd(i),
                  targetRange.strideBegin(i),
                  [](std::vector<uint16_t*> a_, std::vector<uint16_t*> b_) {
                    pairedForEach(a_.begin(), a_.end(), b_.begin(), [](uint16_t* ai_, uint16_t* bi_) { *bi_ = *ai_; });
                  });
  }
  int cnt = 0;
  for (size_t j = 0; j < 4; j++)
    for (size_t i = 0; i < 5; i++)
      REQUIRE(cnt++ == img.calculateIdx({ i, j }));

  cnt = 0;
  for (size_t j = 0; j < 4; j++)
    for (size_t i = 0; i < 5; i++)
      REQUIRE(img[{ i, j }] == *img.getRawPtr(cnt++));
}
