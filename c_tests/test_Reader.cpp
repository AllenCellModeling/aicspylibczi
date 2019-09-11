//
// Created by James Sherman on 9/6/19.
//

#include "catch.hpp"
#include "../_pylibczi/Reader.h"
#include <unistd.h>

class CziCreator{
  std::unique_ptr<pylibczi::Reader> m_czi;
public:
  CziCreator(): m_czi(new pylibczi::Reader( std::fopen("resources/s_1_t_1_c_1_z_1.czi", "rb" ))){}
  pylibczi::Reader * get() { return m_czi.get(); }
};



TEST_CASE("test_reader_constructor", "[Reader]"){
    FILE *fp = std::fopen("resources/s_1_t_1_c_1_z_1.czi", "rb");      // #include <cstdio>
    if(fp == nullptr) std::cout << "failed to open file!" << std::endl;
    REQUIRE_NOTHROW(pylibczi::Reader(fp));

}

TEST_CASE_METHOD(CziCreator, "test_reader_dims_1", "[Reader_Dims]"){
    auto czi = get();
    auto dims = czi->read_dims();
    REQUIRE(dims.size() == 2); // B=0, C=0 for this file
}

TEST_CASE_METHOD(CziCreator, "test_is_mosaic", "[Reader_Is_Mosaic]"){
    auto czi = get();
    REQUIRE( czi->isMosaic() == false);
}

TEST_CASE_METHOD(CziCreator, "test_meta_reader", "[Reader_read_meta]"){
    auto czi = get();
    std::string xml = czi->read_meta();
    std::string ans("<?xml version=\"1.0\"?>\n"
                "<ImageDocument>\n"
                " <Metadata>\n"
                "  <Experiment Version=\"1.1\">\n"
                "   <RunMode>OptimizeBeforePerformEnabled,ValidateAndAdaptBeforePerformEnabled</RunMode>");
    REQUIRE( std::strncmp( ans.c_str(), xml.c_str(), ans.size()) == 0);
}

TEST_CASE_METHOD(CziCreator, "test_read_selected", "[Reader_read_selected]"){
    auto czi = get();
    auto c_dims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B,0 },
                                          { libCZI::DimensionIndex::C,0 } };
    auto imvec = czi->read_selected(c_dims);
    REQUIRE(imvec->size() == 1);
    auto shape = imvec->front()->shape();
    REQUIRE(shape[0] == 325); // height
    REQUIRE(shape[1] == 475); // width
}

// TODO I need a small file for testing mosaic functionality
