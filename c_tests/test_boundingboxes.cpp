#include "catch.hpp"
#include "inc_libCZI.h"
#include <iostream>

#include "Reader.h"
#include "helper_algorithms.h"

using namespace pylibczi;

class CziFile
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziFile()
    : m_czi(new pylibczi::Reader(L"resources/s_3_t_1_c_3_z_5.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziNoScene1
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziNoScene1()
    : m_czi(new pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziNoScene2
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziNoScene2()
    : m_czi(new pylibczi::Reader(L"resources/RGB-8bit.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziMFile
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziMFile()
    : m_czi(new pylibczi::Reader(L"resources/mosaic_test.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziFile, "test_read_tile_bbox", "[Reader_read_tile_bbox]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::S, 0 },
                                       { libCZI::DimensionIndex::C, 0 },
                                       { libCZI::DimensionIndex::Z, 0}
                                      };
  std::vector<libCZI::IntRect> expectedRect{
    {39850, 35568, 475, 325},
    {44851, 35568, 475, 325},
    {39850, 39272, 475, 325}
  };

  Reader::TilePair ans = czi->tileBoundingBox(cDims);

  REQUIRE(ans.second.x == expectedRect[0].x);
  REQUIRE(ans.second.y == expectedRect[0].y);
  REQUIRE(ans.second.w == expectedRect[0].w);
  REQUIRE(ans.second.h == expectedRect[0].h);

  libCZI::IntRect intRect = czi->sceneBoundingBox(0);
  REQUIRE(intRect.x == expectedRect[0].x);
  REQUIRE(intRect.y == expectedRect[0].y);
  REQUIRE(intRect.w == expectedRect[0].w);
  REQUIRE(intRect.h == expectedRect[0].h);

  REQUIRE_THROWS_AS(czi->sceneBoundingBox(3), SceneIndexException);

  auto vals = czi->allSceneBoundingBoxes();

  auto compare = [](const Reader::SceneBBoxMap::value_type &val,
                    const std::vector< libCZI::IntRect >::value_type &expected){
    REQUIRE(val.second.x == expected.x);
    REQUIRE(val.second.y == expected.y);
    REQUIRE(val.second.w == expected.w);
    REQUIRE(val.second.h == expected.h);
  };

  REQUIRE(vals.size() == expectedRect.size());
  pairedForEach(vals.begin(), vals.end(), expectedRect.begin(), compare);

}

TEST_CASE_METHOD(CziNoScene1, "test_read_tile2_bbox", "[Reader_read_tile2_bbox]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::C, 0 },
                                       { libCZI::DimensionIndex::Z, 0}
  };
  libCZI::IntRect expectedRect{ 39856, 39272, 475, 325};
  Reader::TilePair ans = czi->tileBoundingBox(cDims);

  REQUIRE(ans.second.x == expectedRect.x);
  REQUIRE(ans.second.y == expectedRect.y);
  REQUIRE(ans.second.w == expectedRect.w);
  REQUIRE(ans.second.h == expectedRect.h);

  libCZI::IntRect intRect = czi->sceneBoundingBox(0);

  REQUIRE(intRect.x == expectedRect.x);
  REQUIRE(intRect.y == expectedRect.y);
  REQUIRE(intRect.w == expectedRect.w);
  REQUIRE(intRect.h == expectedRect.h);
}

TEST_CASE_METHOD(CziNoScene2, "test_read_tile3_bbox", "[Reader_read_tile3_bbox]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::S, 0 },
                                       { libCZI::DimensionIndex::C, 0 },
                                       { libCZI::DimensionIndex::Z, 0}
  };
  libCZI::IntRect expectedRect{ 0, 0, 924, 624};
  Reader::TilePair ans = czi->tileBoundingBox(cDims);

  REQUIRE(ans.second.x == expectedRect.x);
  REQUIRE(ans.second.y == expectedRect.y);
  REQUIRE(ans.second.w == expectedRect.w);
  REQUIRE(ans.second.h == expectedRect.h);

  libCZI::IntRect intRect = czi->sceneBoundingBox();

  REQUIRE(intRect.x == expectedRect.x);
  REQUIRE(intRect.y == expectedRect.y);
  REQUIRE(intRect.w == expectedRect.w);
  REQUIRE(intRect.h == expectedRect.h);
}

TEST_CASE_METHOD(CziMFile, "test_read_mosaic_bbox", "[Reader_read_mosaic_bbox]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::S, 0 },
                                       { libCZI::DimensionIndex::C, 0 },
                                       { libCZI::DimensionIndex::Z, 0 } };
  libCZI::IntRect expectedRect{ 0, 0, 1756, 624};
  libCZI::IntRect intRect = czi->mosaicBoundingBox();

  REQUIRE(intRect.x == expectedRect.x);
  REQUIRE(intRect.y == expectedRect.y);
  REQUIRE(intRect.w == expectedRect.w);
  REQUIRE(intRect.h == expectedRect.h);
}

TEST_CASE_METHOD(CziMFile, "test_read_mosaic_tile_bbox", "[Reader_read_mosaic_tile_bbox]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::S, 0 },
                                       { libCZI::DimensionIndex::C, 0 },
                                       { libCZI::DimensionIndex::Z, 0 } };
  {
    libCZI::IntRect expectedRect{ 0, 0,  924, 624 };
    Reader::TilePair ans = czi->mosaicTileBoundingBox(cDims, 0);

    REQUIRE(ans.second.x == expectedRect.x);
    REQUIRE(ans.second.y == expectedRect.y);
    REQUIRE(ans.second.w == expectedRect.w);
    REQUIRE(ans.second.h == expectedRect.h);
  }
  {
    libCZI::IntRect expectedRect{ 832, 0, 924, 624 };
    Reader::TilePair ans = czi->mosaicTileBoundingBox(cDims, 1);

    REQUIRE(ans.second.x == expectedRect.x);
    REQUIRE(ans.second.y == expectedRect.y);
    REQUIRE(ans.second.w == expectedRect.w);
    REQUIRE(ans.second.h == expectedRect.h);
  }

  REQUIRE_THROWS_AS(czi->mosaicTileBoundingBox(cDims, 2), CDimCoordinatesOverspecifiedException);
}
