//
// Created by Jamie Sherman on 2019-09-10.
//

#include "catch.hpp"
#include "Image.h"
#include "Reader.h"
#include "exceptions.h"
#include "Iterator.h"
#include "helper_algorithms.h"

using namespace pylibczi;

TEST_CASE("imagefactory_pixeltype", "[ImageFactory_PixelType]")
{
	REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Gray8)==1);
	REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Gray16)==2);
	REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Bgr24)==1);
	REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Bgr48)==2);
	REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Gray32Float)==4);
	REQUIRE(ImageFactory::size_of_pixel_type(libCZI::PixelType::Bgr96Float)==4);
}

TEST_CASE("imagefactory_nofchannels", "[ImageFactory_NofChannels]")
{
	REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Gray8)==1);
	REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Gray16)==1);
	REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Bgr24)==3);
	REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Bgr48)==3);
	REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Gray32Float)==1);
	REQUIRE(ImageFactory::n_of_channels(libCZI::PixelType::Bgr96Float)==3);
}

class CziImageCreator {
	std::unique_ptr<Reader> m_czi;
public:
	CziImageCreator()
			:m_czi(new Reader(std::fopen("resources/s_1_t_1_c_1_z_1.czi", "rb"))) { }
	std::shared_ptr<ImageBC> get()
	{
		auto c_dims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
		                                     {libCZI::DimensionIndex::C, 0}};
		auto imvec = m_czi->read_selected(c_dims).first;
		return imvec.front();
	}
};

TEST_CASE_METHOD(CziImageCreator, "test_image_cast", "[Image_Cast]")
{
	std::shared_ptr<ImageBC> img = get();
	REQUIRE(img.get()->is_type_match<uint16_t>());
	REQUIRE(!(img->is_type_match<uint8_t>()));
	REQUIRE(!(img->is_type_match<float>()));
}

TEST_CASE_METHOD(CziImageCreator, "test_image_throw", "[Image_Cast_Throw]")
{
	std::shared_ptr<ImageBC> img = get();
	REQUIRE_THROWS_AS(ImageFactory::get_derived<uint8_t>(img), PixelTypeException);
}

TEST_CASE_METHOD(CziImageCreator, "test_image_nothrow", "[Image_Cast_Nothrow]")
{
	std::shared_ptr<ImageBC> img = get();
	REQUIRE_NOTHROW(ImageFactory::get_derived<uint16_t>(img));
}

TEST_CASE("test_image_accessors", "[Image_operator[]]")
{
	libCZI::CDimCoordinate cdim{{libCZI::DimensionIndex::S, 1}, {libCZI::DimensionIndex::C, 1}};
	Image<uint16_t> img({3, 4, 5}, libCZI::PixelType::Gray16, &cdim, {0, 0, 5, 4}, -1);
	uint16_t ip[60];
	for (int i = 0; i<3*4*5; i++) ip[i] = i/3+1;

	pylibczi::SourceRange<uint16_t> sourceRange(3, ip, ip+60, 30, 5);
	pylibczi::TargetRange<uint16_t> targetRange(3, 5, 4, img.get_raw_ptr(), img.get_raw_ptr(60));

	pylibczi::SourceRange<uint16_t>::source_channel_iterator beg = sourceRange.stride_begin(1);
	pylibczi::SourceRange<uint16_t>::source_channel_iterator end = sourceRange.stride_end(0);
	REQUIRE(beg==end);
	REQUIRE(sourceRange.stride_begin(2)==sourceRange.stride_end(1));
	REQUIRE(sourceRange.stride_begin(3)==sourceRange.stride_end(2));

	for (int i = 0; i<4; i++) { // copy stride by stride as you would with an actual image
		paired_for_each(sourceRange.stride_begin(i), sourceRange.stride_end(i), targetRange.stride_begin(i),
				[](std::vector<uint16_t*> a, std::vector<uint16_t*> b) {
					paired_for_each(a.begin(), a.end(), b.begin(), [](uint16_t* ai, uint16_t* bi) {
						*bi = *ai;
					});
				});
	}
	int cnt = 0;
	for (size_t k = 0; k<3; k++)
		for (size_t j = 0; j<4; j++)
			for (size_t i = 0; i<5; i++)
				REQUIRE(cnt++==img.calculate_idx({i, j, k}));

	cnt = 0;
	for (size_t k = 0; k<3; k++)
		for (size_t j = 0; j<4; j++)
			for (size_t i = 0; i<5; i++)
				REQUIRE(img[{i, j, k}]==*img.get_raw_ptr(cnt++));
}

TEST_CASE("test_image_accessors_2d", "[Image_operator[2d]]")
{
	libCZI::CDimCoordinate cdim{{libCZI::DimensionIndex::S, 1}, {libCZI::DimensionIndex::C, 1}};
	Image<uint16_t> img({4, 5}, libCZI::PixelType::Gray16, &cdim, {0, 0, 5, 4}, -1);
	uint16_t ip[20];
	for (int i = 0; i<4*5; i++) ip[i] = i+1;

	pylibczi::SourceRange<uint16_t> sourceRange(1, ip, ip+20, 10, 5);
	pylibczi::TargetRange<uint16_t> targetRange(1, 5, 4, img.get_raw_ptr(), img.get_raw_ptr(20));

	pylibczi::SourceRange<uint16_t>::source_channel_iterator beg = sourceRange.stride_begin(1);
	pylibczi::SourceRange<uint16_t>::source_channel_iterator end = sourceRange.stride_end(0);
	REQUIRE(beg==end);
	REQUIRE(sourceRange.stride_begin(2)==sourceRange.stride_end(1));
	REQUIRE(sourceRange.stride_begin(3)==sourceRange.stride_end(2));

	for (int i = 0; i<4; i++) { // copy stride by stride as you would with an actual image
		paired_for_each(sourceRange.stride_begin(i), sourceRange.stride_end(i), targetRange.stride_begin(i),
				[](std::vector<uint16_t*> a, std::vector<uint16_t*> b) {
					paired_for_each(a.begin(), a.end(), b.begin(), [](uint16_t* ai, uint16_t* bi) {
						*bi = *ai;
					});
				});
	}
	int cnt = 0;
	for (size_t j = 0; j<4; j++)
		for (size_t i = 0; i<5; i++)
			REQUIRE(cnt++==img.calculate_idx({i, j}));

	cnt = 0;
	for (size_t j = 0; j<4; j++)
		for (size_t i = 0; i<5; i++)
			REQUIRE(img[{i, j}]==*img.get_raw_ptr(cnt++));
}

