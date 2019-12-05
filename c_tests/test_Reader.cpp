#include <cstdio>
#include <algorithm>

#include "catch.hpp"

#include "../_pylibczi/Reader.h"
#include "../_pylibczi/pb_helpers.h"

class CziCreator {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreator()
        : m_czi(new pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi")) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreator2 {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreator2()
        : m_czi(new pylibczi::Reader(L"resources/s_3_t_1_c_3_z_5.czi")) {}
    pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreatorIStream {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreatorIStream()
        :m_czi()
    {
        auto fp = std::shared_ptr<libCZI::IStream>(new CSimpleStreamImplCppStreams(L"resources/s_1_t_1_c_1_z_1.czi"));
        m_czi = std::unique_ptr< pylibczi::Reader >( new pylibczi::Reader(fp) );
    }
    pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziCreatorIStream, "test_reader_constructor", "[Reader]")
{
    REQUIRE_NOTHROW(get());
}

TEST_CASE("open_two_czis", "[Reader_Dup]")
{
    REQUIRE_NOTHROW(pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi"));
    REQUIRE_NOTHROW(pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi"));
}

TEST_CASE_METHOD(CziCreator, "test_reader_dims_1", "[Reader_Dims]")
{
    auto czi = get();
    auto dims = czi->readDims();
    auto cDim = find_if(dims.begin(), dims.end(),[](const auto &pr_){ return pr_.first == 'C'; });
    REQUIRE(cDim->second == std::pair<int, int>(0, 0));
    REQUIRE(dims.size()==4); // B=0, C=0, Y, X for this file
}

TEST_CASE_METHOD(CziCreator, "test_reader_dims_2", "[Reader_Dims_String]")
{
    auto czi = get();
    auto dims = czi->dimsString();
    REQUIRE(dims == std::string("BCYX")); // B=0, C=0 for this file
}

TEST_CASE_METHOD(CziCreator2, "test_reader_dims_3", "[Reader_Dims_Size]")
{
    auto czi = get();
    std::string dstr = czi->dimsString();
    auto dims = czi->dimSizes();
    //                                  B  S  C  Z
    std::initializer_list<int> shape = {1, 3, 3, 5, 325, 475};
    pairedForEach(dims.begin(), dims.end(), shape.begin(), [](int a_, int b_){
        REQUIRE( a_ == b_);
    });
    REQUIRE( dstr == "BSCZYX");
}

TEST_CASE_METHOD(CziCreator, "test_is_mosaic", "[Reader_Is_Mosaic]")
{
    auto czi = get();
    REQUIRE(!czi->isMosaic());
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
    auto cDims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                         {libCZI::DimensionIndex::C, 0}};
    auto imvec = czi->readSelected(cDims).first;
    REQUIRE(imvec.size()==1);
    auto shape = imvec.front()->shape();
    REQUIRE(shape[0]==325); // height
    REQUIRE(shape[1]==475); // width
}

TEST_CASE_METHOD(CziCreator2, "test_read_selected2", "[Reader_read_selected]")
{
    auto czi = get();
    auto cDims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                        {libCZI::DimensionIndex::C, 0}};
    auto imvec = czi->readSelected(cDims).first;
    REQUIRE(imvec.size()==15);
    auto shape = imvec.front()->shape();
    REQUIRE(shape[0]==325); // height
    REQUIRE(shape[1]==475); // width
}

TEST_CASE_METHOD(CziCreatorIStream, "test_read_selected3", "[Reader_read_selected]")
{
    auto czi = get();
    auto cDims = libCZI::CDimCoordinate{ {libCZI::DimensionIndex::C, 0}};
    auto imvec = czi->readSelected(cDims).first;
    REQUIRE(imvec.size()==1);
    auto shape = imvec.front()->shape();
    REQUIRE(shape[0]==325); // height
    REQUIRE(shape[1]==475); // width
}

TEST_CASE_METHOD(CziCreatorIStream, "test_read_selected4", "[Reader_read_selected]")
{
    auto czi = get();
    auto cDims = libCZI::CDimCoordinate();
    auto imvec = czi->readSelected(cDims).first;
    REQUIRE(imvec.size()==1);
    auto shape = imvec.front()->shape();
    REQUIRE(shape[0]==325); // height
    REQUIRE(shape[1]==475); // width
}

TEST_CASE_METHOD(CziCreator2, "test_read_subblock_meta", "[Reader_read_subblock_meta]")
{
    auto czi = get();
    auto cDims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                        {libCZI::DimensionIndex::C, 0}};
    auto metavec = czi->readSubblockMeta(cDims);
    REQUIRE(metavec.size() == 15);
    auto test = pb_helpers::packStringArray(metavec);
    int x = 10;
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
