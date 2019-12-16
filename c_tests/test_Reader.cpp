#include <cstdio>
#include <algorithm>

#include "catch.hpp"

#include "../_aicspylibczi/exceptions.h"
#include "../_aicspylibczi/Reader.h"
#include "../_aicspylibczi/pb_helpers.h"

class CziCreator {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreator()
        :m_czi(new pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi")) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreator2 {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreator2()
        :m_czi(new pylibczi::Reader(L"resources/s_3_t_1_c_3_z_5.czi")) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreatorIStream {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziCreatorIStream()
        :m_czi()
    {
        auto fp = std::shared_ptr<libCZI::IStream>(new CSimpleStreamImplCppStreams(L"resources/s_1_t_1_c_1_z_1.czi"));
        m_czi = std::unique_ptr<pylibczi::Reader>(new pylibczi::Reader(fp));
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
    auto cDim = find_if(dims.begin(), dims.end(), [](const auto& pr_) { return pr_.first=='C'; });
    REQUIRE(cDim->second==std::pair<int, int>(0, 0));
    REQUIRE(dims.size()==4); // B=0, C=0, Y, X for this file
}

TEST_CASE_METHOD(CziCreator, "test_reader_dims_2", "[Reader_Dims_String]")
{
    auto czi = get();
    auto dims = czi->dimsString();
    REQUIRE(dims==std::string("BCYX")); // B=0, C=0 for this file
}

TEST_CASE_METHOD(CziCreator2, "test_reader_dims_3", "[Reader_Dims_Size]")
{
    auto czi = get();
    std::string dstr = czi->dimsString();
    auto dims = czi->dimSizes();
    //                                  B  S  C  Z
    std::initializer_list<int> shape = {1, 3, 3, 5, 325, 475};
    pairedForEach(dims.begin(), dims.end(), shape.begin(), [](int a_, int b_) {
        REQUIRE(a_==b_);
    });
    REQUIRE(dstr=="BSCZYX");
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
    auto cDims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::C, 0}};
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

TEST_CASE_METHOD(CziCreator2, "test_bad_scene", "[Reader_read_bad_scene]")
{
    auto czi = get();
    auto cDims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                        {libCZI::DimensionIndex::C, 0},
                                        {libCZI::DimensionIndex::S, 4}};
    REQUIRE_THROWS_AS(czi->readSelected(cDims), pylibczi::CDimCoordinatesOverspecifiedException);
}

TEST_CASE_METHOD(CziCreator2, "test_read_subblock_meta", "[Reader_read_subblock_meta]")
{
    auto czi = get();
    auto cDims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                        {libCZI::DimensionIndex::C, 0}};
    auto metavec = czi->readSubblockMeta(cDims);
    REQUIRE(metavec.size()==15);
    auto test = pb_helpers::packStringArray(metavec);
    int x = 10;
}

// TODO I need a small file for testing mosaic functionality

class CziMCreator {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziMCreator()
        :m_czi(new pylibczi::Reader(L"resources/mosaic_test.czi")) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziMCreator, "test_mosaic_is_mosaic", "[Reader_mosaic_is_mosaic]")
{
    auto czi = get();
    REQUIRE(czi->isMosaic());
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_dims", "[Reader_mosaic_dims]")
{
    auto czi = get();
    REQUIRE(czi->dimsString() == std::string("STCZMYX"));
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_dimsSize", "[Reader_mosaic_dimsSize]")
{
    auto czi = get();
    std::vector<int> ans{1, 1, 1, 1, 2, 624, 924};
    REQUIRE(czi->dimSizes() == ans);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_readdims", "[Reader_mosaic_readdims]")
{
    auto czi = get();
    pylibczi::Reader::DimensionRangeMap ans{{'S', {0, 0}}, {'T', {0, 0}}, {'C', {0, 0}},
                                            {'Z', {0, 0}}, {'M', {0, 1}},
                                            {'Y', {0, 623}}, {'X', {0, 923}} };
    auto val = czi->readDims();
    auto vitt = val.begin();
    auto aitt = ans.begin();
    for(; aitt != ans.end() && vitt != val.end(); aitt++, vitt++) {
        REQUIRE(vitt->first == aitt->first);
        REQUIRE(vitt->second.first == aitt->second.first);
        REQUIRE(vitt->second.second == aitt->second.second);
    }
    REQUIRE(val == ans);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_readSelected", "[Reader_mosaic_readSelected]")
{
    auto czi = get();
    REQUIRE(czi->dimsString()==std::string("STCZMYX"));
    auto sze = czi->dimSizes();
    std::vector<int> szeAns{1, 1, 1, 1, 2, 624, 924};
    REQUIRE(sze==szeAns);
    libCZI::CDimCoordinate c_dims;
    auto imvec = czi->readSelected(c_dims);
    REQUIRE(imvec.first.size()==2);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_read", "[Reader_mosaic_read]")
{
    auto czi = get();
    auto c_dims = libCZI::CDimCoordinate{{libCZI::DimensionIndex::C, 0}};
    auto imvec = czi->readMosaic(c_dims);
    REQUIRE(imvec.size()==1);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_shape", "[Reader_mosaic_shape]")
{
    auto czi = get();
    auto bbox = czi->mosaicShape();
    REQUIRE(bbox.w==1756);
    REQUIRE(bbox.h==624);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_throw", "[Reader_mosaic_throw]")
{
    auto czi = get();
    libCZI::CDimCoordinate c_dims;
    REQUIRE_THROWS_AS(czi->readMosaic(c_dims), pylibczi::CDimCoordinatesUnderspecifiedException);
}

class CziBgrCreator {
    std::unique_ptr<pylibczi::Reader> m_czi;
public:
    CziBgrCreator()
        :m_czi(new pylibczi::Reader(L"resources/RGB-8bit.czi")) { }
    pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziBgrCreator, "test_bgr_read", "[Reader_read_bgr]")
{
    auto czi = get();

    libCZI::CDimCoordinate dm;
    auto pr = czi->readSelected(dm);

    REQUIRE(czi->dimsString()==std::string("TYX"));
    std::vector<int> ansSize{1, 624, 924};
    REQUIRE(czi->dimSizes()==ansSize);

    auto dims = czi->readDims();
    pylibczi::Reader::DimensionRangeMap ansDims{{'T', {0, 0}}, {'Y', {0, 623}}, {'X', {0, 923}}};
    REQUIRE(!ansDims.empty());
    REQUIRE(!dims.empty());
    REQUIRE(dims==ansDims);

    REQUIRE(pr.first.size()==1);
    REQUIRE(pr.first.front()->shape()[1]==624);
    REQUIRE(pr.first.front()->shape()[2]==924);
    REQUIRE(pr.first.front()->pixelType()==libCZI::PixelType::Bgr24);
}

TEST_CASE_METHOD(CziBgrCreator, "test_bgr_flatten", "[Reader_read_flatten_bgr]")
{
    auto czi = get();

    auto dims = czi->readDims();

    libCZI::CDimCoordinate dm;
    auto pr = czi->readSelected(dm, -1, true);
    REQUIRE(pr.first.size()==3);

    for (auto x : pr.first) {
        REQUIRE(x->shape()[0]==624);
        REQUIRE(x->shape()[1]==924);
    }

    pylibczi::Reader::Shape shapeAns{{'B', 1}, {'T', 1}, {'C', 3}, {'Y', 624}, {'X', 924}};
    REQUIRE(pr.second==shapeAns);
    REQUIRE(pr.first.front()->pixelType()==libCZI::PixelType::Gray8);
    // pb_helpers::packArray(pr.first);
}
