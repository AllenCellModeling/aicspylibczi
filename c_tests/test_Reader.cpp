#include "catch.hpp"
#include "../_pylibczi/Reader.h"
#include <cstdio>
#include "pb_helpers.h"

class CziCreator {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreator()
        :m_czi(new pylibczi::Reader(std::fopen("resources/s_1_t_1_c_1_z_1.czi", "rb"))) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreator2 {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreator2()
        :m_czi(new pylibczi::Reader(std::fopen("resources/s_3_t_1_c_3_z_5.czi", "rb"))) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

/* local test - file is 30gb
class CziMCreator{
	std::unique_ptr<pylibczi::Reader> m_czi;
public:
	CziMCreator(): m_czi(new pylibczi::Reader( std::fopen("/Users/jamies/Data/20190618_CL001_HB01_Rescan_002.czi", "rb" ))){}
	pylibczi::Reader * get() { return m_czi.get(); }
};
*/

TEST_CASE("test_reader_constructor", "[Reader]")
{
    FILE* fp = std::fopen("resources/s_1_t_1_c_1_z_1.czi", "rb");      // #include <cstdio>
    if (fp==nullptr) std::cout << "failed to open file!" << std::endl;
    REQUIRE_NOTHROW(pylibczi::Reader(fp));

}

TEST_CASE_METHOD(CziCreator, "test_reader_dims_1", "[Reader_Dims]")
{
    auto czi = get();
    auto dims = czi->readDims();
    REQUIRE(dims.size()==2); // B=0, C=0 for this file
}

TEST_CASE_METHOD(CziCreator, "test_is_mosaic", "[Reader_Is_Mosaic]")
{
    auto czi = get();
    REQUIRE(czi->isMosaic()==false);
}

TEST_CASE_METHOD(CziCreator, "test_meta_reader", "[Reader_read_meta]")
{
    auto czi = get();
    std::string xml = czi->readMeta();
    std::string ans("<?xml version=\"1.0\"?>\n"
                    "<ImageDocument>\n"
                    " <Metadata>\n"
                    "  <Experiment Version=\"1.1\">\n"
                    "   <RunMode>OptimizeBeforePerformEnabled,ValidateAndAdaptBeforePerformEnabled</RunMode>");
    REQUIRE(std::strncmp(ans.c_str(), xml.c_str(), ans.size())==0);
}

TEST_CASE_METHOD(CziCreator, "test_read_selected", "[Reader_read_selected]")
{
    auto czi = get();
    auto c_dims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                         {libCZI::DimensionIndex::C, 0}};
    auto imvec = czi->readSelected(c_dims).first;
    REQUIRE(imvec.size()==1);
    auto shape = imvec.front()->shape();
    REQUIRE(shape[0]==325); // height
    REQUIRE(shape[1]==475); // width
}

TEST_CASE_METHOD(CziCreator2, "test_read_selected2", "[Reader_read_selected]")
{
    auto czi = get();
    auto c_dims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                         {libCZI::DimensionIndex::C, 0}};
    auto imvec = czi->readSelected(c_dims).first;
    REQUIRE(imvec.size()==15);
    auto shape = imvec.front()->shape();
    REQUIRE(shape[0]==325); // height
    REQUIRE(shape[1]==475); // width
}

// TODO I need a small file for testing mosaic functionality

/* The file is 30GB can't use it for server test
TEST_CASE_METHOD(CziMCreator, "test_read_mosaic", "[Reader_read_mosaic]"){
	auto czi = get();
	auto c_dims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::C,0 } };
	auto imvec = czi->readMosaic(c_dims);
	REQUIRE(imvec.size() == 1);
	//auto shape = imvec.front()->shape();
	//REQUIRE(shape[0] == 325); // height
	//REQUIRE(shape[1] == 475); // width
}
*/
