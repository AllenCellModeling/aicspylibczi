#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>

#include "catch.hpp"

#include "../_aicspylibczi/Reader.h"
#include "../_aicspylibczi/exceptions.h"
#include "../_aicspylibczi/pb_helpers.h"

#define CORES_FOR_THREADS                                                      \
  1 // for local testing increase this but for GH Actions it must be 1

class CziCreator
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreator()
    : m_czi(new pylibczi::Reader(L"resources/s_1_t_1_c_1_z_1.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreator2
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreator2()
    : m_czi(new pylibczi::Reader(L"resources/s_3_t_1_c_3_z_5.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreatorIStream
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreatorIStream()
    : m_czi()
  {
    auto fp = std::shared_ptr<libCZI::IStream>(
      new CSimpleStreamImplCppStreams(L"resources/s_1_t_1_c_1_z_1.czi"));
    m_czi = std::unique_ptr<pylibczi::Reader>(new pylibczi::Reader(fp));
  }
  pylibczi::Reader* get() { return m_czi.get(); }
};

#ifdef LOCAL_TEST

class CziCreatorBig
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreatorBig()
    : m_czi(new pylibczi::Reader(
        L"/Users/jamies/Data/20190425_S08_001-04-Scene-4-P3-B03.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreatorBigM
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreatorBigM()
    : m_czi(new pylibczi::Reader(L"/Users/jamies/Data/20190614_C01_001.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};
#endif

class CziCreator4
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreator4()
    : m_czi(new pylibczi::Reader(L"resources/s_1_t_10_c_3_z_1.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

class CziCreator5
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziCreator5()
    : m_czi(new pylibczi::Reader(L"resources/Multiscene_CZI_3Scenes.czi"))
  {}
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
  using DI = pylibczi::DimIndex;
  pylibczi::Reader::DimsShape ans{
    { { DI::X, { 0, 475 } },
      { DI::Y, { 0, 325 } },
      { DI::C, { 0, 1 } },
      { DI::B, { 0, 1 } } },
  };
  auto czi = get();
  auto dimsVec = czi->readDimsRange();
  REQUIRE(czi->shapeIsConsistent());
  REQUIRE(dimsVec.size() == 1);
  REQUIRE(dimsVec == ans);
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
  std::vector<int> shape = { 1, 3, 3, 5, 325, 475 };
  REQUIRE(czi->shapeIsConsistent());
  REQUIRE(dstr == "BSCZYX");
  REQUIRE(dims == shape);
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
                  "   "
                  "<RunMode>OptimizeBeforePerformEnabled,"
                  "ValidateAndAdaptBeforePerformEnabled</RunMode>");
  REQUIRE(std::strncmp(ans.c_str(), xml.c_str(), ans.size()) == 0);
}

TEST_CASE_METHOD(CziCreator, "test_read_selected", "[Reader_read_selected]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::C, 0 } };
  auto imCont = czi->readSelected(cDims, -1, CORES_FOR_THREADS);
  auto imvec = imCont.first->images();
  REQUIRE(imvec.size() == 1);
  auto shape = imvec.front()->shape();
  REQUIRE(shape[0] == 325); // height
  REQUIRE(shape[1] == 475); // width
}

TEST_CASE_METHOD(CziCreator2, "test_read_selected2", "[Reader_read_selected]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::C, 0 } };
  auto imCont = czi->readSelected(cDims, -1, CORES_FOR_THREADS);
  auto imvec = imCont.first->images();
  REQUIRE(imvec.size() == 15);
  auto shape = imvec.front()->shape();
  REQUIRE(shape[0] == 325); // height
  REQUIRE(shape[1] == 475); // width
}

TEST_CASE_METHOD(CziCreatorIStream,
                 "test_read_selected3",
                 "[Reader_read_selected]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::C, 0 } };
  auto imCont = czi->readSelected(cDims, -1, CORES_FOR_THREADS);
  auto imvec = imCont.first->images();
  REQUIRE(imvec.size() == 1);
  auto shape = imvec.front()->shape();
  REQUIRE(shape[0] == 325); // height
  REQUIRE(shape[1] == 475); // width
}

TEST_CASE_METHOD(CziCreatorIStream,
                 "test_read_selected4",
                 "[Reader_read_selected]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate();
  auto imCont = czi->readSelected(cDims, -1, CORES_FOR_THREADS);
  auto imvec = imCont.first->images();
  REQUIRE(imvec.size() == 1);
  auto shape = imvec.front()->shape();
  REQUIRE(shape[0] == 325); // height
  REQUIRE(shape[1] == 475); // width
}

TEST_CASE_METHOD(CziCreator2, "test_bad_scene", "[Reader_read_bad_scene]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::C, 0 },
                                       { libCZI::DimensionIndex::S, 4 } };
  REQUIRE_THROWS_AS(czi->readSelected(cDims, -1, CORES_FOR_THREADS),
                    pylibczi::CDimCoordinatesOverspecifiedException);
}

TEST_CASE_METHOD(CziCreator2,
                 "test_read_subblock_meta",
                 "[Reader_read_subblock_meta]")
{
  auto czi = get();
  auto cDims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                                       { libCZI::DimensionIndex::C, 0 } };
  auto metavec = czi->readSubblockMeta(cDims);
  REQUIRE(metavec.size() == 15);
  auto test = pb_helpers::packStringArray(metavec);
  int x = 10;
}

class CziMCreator
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziMCreator()
    : m_czi(new pylibczi::Reader(L"resources/mosaic_test.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziMCreator,
                 "test_mosaic_is_mosaic",
                 "[Reader_mosaic_is_mosaic]")
{
  auto czi = get();
  REQUIRE(czi->isMosaic());
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_dims", "[Reader_mosaic_dims]")
{
  auto czi = get();
  REQUIRE(czi->dimsString() == std::string("STCZMYX"));
}

TEST_CASE_METHOD(CziMCreator,
                 "test_mosaic_dimsSize",
                 "[Reader_mosaic_dimsSize]")
{
  auto czi = get();
  std::vector<int> ans{ 1, 1, 1, 1, 2, 624, 924 };
  REQUIRE(czi->dimSizes() == ans);
}

TEST_CASE_METHOD(CziMCreator,
                 "test_mosaic_readdims",
                 "[Reader_mosaic_readdims]")
{
  auto czi = get();
  using DI = pylibczi::DimIndex;
  pylibczi::Reader::DimsShape ans{ { { DI::S, { 0, 1 } },
                                     { DI::T, { 0, 1 } },
                                     { DI::C, { 0, 1 } },
                                     { DI::Z, { 0, 1 } },
                                     { DI::M, { 0, 2 } },
                                     { DI::Y, { 0, 624 } },
                                     { DI::X, { 0, 924 } } } };
  auto val = czi->readDimsRange();
  REQUIRE(val == ans);
}

TEST_CASE_METHOD(CziMCreator,
                 "test_mosaic_readSelected",
                 "[Reader_mosaic_readSelected]")
{
  auto czi = get();
  REQUIRE(czi->dimsString() == std::string("STCZMYX"));
  auto sze = czi->dimSizes();
  std::vector<int> szeAns{ 1, 1, 1, 1, 2, 624, 924 };
  REQUIRE(sze == szeAns);
  libCZI::CDimCoordinate c_dims;
  auto imCont = czi->readSelected(c_dims, -1, CORES_FOR_THREADS);
  auto imvec = imCont.first->images();
  REQUIRE(imvec.size() == 2);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_read", "[Reader_mosaic_read]")
{
  auto czi = get();
  auto c_dims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::C, 0 } };
  auto imCont = czi->readMosaic(c_dims);
  auto imvec = imCont->images();
  REQUIRE(imvec.size() == 1);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_shape", "[Reader_mosaic_shape]")
{
  auto czi = get();
  auto bbox = czi->mosaicShape();
  REQUIRE(bbox.w == 1756);
  REQUIRE(bbox.h == 624);
}

TEST_CASE_METHOD(CziMCreator, "test_mosaic_throw", "[Reader_mosaic_throw]")
{
  auto czi = get();
  libCZI::CDimCoordinate c_dims;
  REQUIRE_THROWS_AS(czi->readMosaic(c_dims),
                    pylibczi::CDimCoordinatesUnderspecifiedException);
}

class CziBgrCreator
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziBgrCreator()
    : m_czi(new pylibczi::Reader(L"resources/RGB-8bit.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziBgrCreator, "test_bgr_read", "[Reader_read_bgr]")
{
  auto czi = get();
  libCZI::CDimCoordinate dm;
  auto imgCont = czi->readSelected(dm, -1, CORES_FOR_THREADS);
  auto pr = imgCont.first->images();

  REQUIRE(czi->dimsString() == std::string("TYX"));
  std::vector<int> ansSize{ 1, 624, 924 };
  REQUIRE(czi->dimSizes() == ansSize);

  using DI = pylibczi::DimIndex;
  pylibczi::Reader::DimsShape ansDims{
    { { DI::T, { 0, 1 } }, { DI::Y, { 0, 624 } }, { DI::X, { 0, 924 } } }
  };
  auto dims = czi->readDimsRange();
  REQUIRE(!dims.empty());
  REQUIRE(dims == ansDims);

  REQUIRE(pr.size() == 1);
  REQUIRE(pr.front()->shape() == std::vector<size_t>{ 3, 624, 924 });
  REQUIRE(pr.front()->pixelType() == libCZI::PixelType::Gray8);
}

TEST_CASE_METHOD(CziBgrCreator, "test_bgr_flatten", "[Reader_read_flatten_bgr]")
{
  auto czi = get();

  auto dims = czi->readDimsRange();

  libCZI::CDimCoordinate dm;
  auto imgCont = czi->readSelected(dm, -1, CORES_FOR_THREADS);
  auto pr = imgCont.first->images();
  auto shape = imgCont.second;
  REQUIRE(pr.size() == 1);

  for (auto x : pr) {
    REQUIRE(x->shape()[0] == 3);
    REQUIRE(x->shape()[1] == 624);
    REQUIRE(x->shape()[2] == 924);
  }

  pylibczi::Reader::Shape shapeAns{
    { 'T', 1 }, { 'C', 3 }, { 'Y', 624 }, { 'X', 924 }
  };

  REQUIRE(shape == shapeAns);
  REQUIRE(pr.front()->pixelType() == libCZI::PixelType::Gray8);
}

TEST_CASE_METHOD(CziMCreator,
                 "test_reader_mosaic_subblockinforect",
                 "[Reader_Mosaic_SubblockInfo_Rect]")
{
  auto czi = get();

  std::vector<libCZI::IntRect> answers{ { 0, 0, 924, 624 },
                                        { 832, 0, 924, 624 } };

  libCZI::CDimCoordinate dm;
  for (int m_index = 0; m_index < 2; m_index++) {
    auto rect = czi->readSubblockRect(dm, m_index);
    auto answer = answers[m_index];
    REQUIRE(rect.x == answer.x);
    REQUIRE(rect.y == answer.y);
    REQUIRE(rect.w == answer.w);
    REQUIRE(rect.h == answer.h);
  }
  int invalid_m = 2;
  REQUIRE_THROWS_AS(czi->readSubblockRect(dm, invalid_m),
                    pylibczi::CDimCoordinatesOverspecifiedException);
}

TEST_CASE_METHOD(CziCreator2,
                 "test_reader_subblockinforect",
                 "[Reader_Std_SubblockInfo_Rect]")
{
  auto czi = get();

  std::vector<libCZI::IntRect> answers{ { 39850, 35568, 475, 325 },
                                        { 44851, 35568, 475, 325 },
                                        { 39850, 39272, 475, 325 } };

  for (int s_index = 0; s_index < 3; s_index++) {
    libCZI::CDimCoordinate dm{ { libCZI::DimensionIndex::S, s_index } };
    auto rect = czi->readSubblockRect(dm);
    auto ans = answers[s_index];
    REQUIRE(rect.x == ans.x);
    REQUIRE(rect.y == ans.y);
    REQUIRE(rect.w == ans.w);
    REQUIRE(rect.h == ans.h);
  }

  libCZI::CDimCoordinate invalid_dim{ { libCZI::DimensionIndex::S, 5 } };
  REQUIRE_THROWS_AS(czi->readSubblockRect(invalid_dim),
                    pylibczi::CDimCoordinatesOverspecifiedException);
}

// Test multichannel BGR
class CziBgrCreator2
{
  std::unique_ptr<pylibczi::Reader> m_czi;

public:
  CziBgrCreator2()
    : m_czi(new pylibczi::Reader(L"resources/RGB-multichannel.czi"))
  {}
  pylibczi::Reader* get() { return m_czi.get(); }
};

TEST_CASE_METHOD(CziBgrCreator2, "test_bgr2_read", "[Reader_read_bgr2]")
{
  auto czi = get();
  libCZI::CDimCoordinate dm =
    libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                            { libCZI::DimensionIndex::C, 4 } };
  auto imgCont = czi->readSelected(dm, -1, CORES_FOR_THREADS);
  auto pr = imgCont.first->images();
  auto shape = imgCont.second;

  REQUIRE(czi->dimsString() == std::string("SCYX"));
  std::vector<int> ansSize{ 1, 7, 81, 147 };
  REQUIRE(czi->dimSizes() == ansSize);

  using DI = pylibczi::DimIndex;
  pylibczi::Reader::DimsShape ansDims{ { { DI::S, { 0, 1 } },
                                         { DI::C, { 0, 7 } },
                                         { DI::Y, { 0, 81 } },
                                         { DI::X, { 0, 147 } } } };
  auto dims = czi->readDimsRange();
  REQUIRE(!dims.empty());
  REQUIRE(dims == ansDims);

  REQUIRE(pr.size() == 1);
  REQUIRE(pr.front()->shape() == std::vector<size_t>{ 3, 81, 147 });
  REQUIRE(pr.front()->pixelType() == libCZI::PixelType::Gray8);
  auto c_pair =
    std::find_if(shape.begin(), shape.end(), [](const std::pair<char, int>& a) {
      return a.first == 'C';
    });
  // TODO: Figure out how to report channels in BGR Images.
}

TEST_CASE_METHOD(CziBgrCreator2,
                 "test_bgr2_flatten",
                 "[Reader_read_flatten_bgr2]")
{
  auto czi = get();

  auto dims = czi->readDimsRange();
  libCZI::CDimCoordinate dm =
    libCZI::CDimCoordinate{ { libCZI::DimensionIndex::C, 4 } };
  auto imgCont = czi->readSelected(dm, -1, CORES_FOR_THREADS);
  auto pr = imgCont.first->images();
  auto shape = imgCont.second;

  REQUIRE(pr.size() == 1);

  for (auto x : pr) {
    REQUIRE(x->shape()[0] == 3);
    REQUIRE(x->shape()[1] == 81);
    REQUIRE(x->shape()[2] == 147);
  }

  pylibczi::Reader::Shape shapeAns{
    { 'S', 1 }, { 'C', 3 }, { 'Y', 81 }, { 'X', 147 }
  };
  REQUIRE(shape == shapeAns);
  REQUIRE(pr.front()->pixelType() == libCZI::PixelType::Gray8);
}

TEST_CASE_METHOD(CziBgrCreator2, "test_bgr_7channel", "[Reader_bgr_7channel]")
{
  auto czi = get();
  libCZI::CDimCoordinate dm;
  auto imCont = czi->readSelected(dm, -1, CORES_FOR_THREADS);
  auto images = imCont.first->images();
  auto shape = imCont.second;
  pylibczi::Reader::Shape shapeAns{
    { 'S', 1 }, { 'C', 21 }, { 'Y', 81 }, { 'X', 147 }
  };
  REQUIRE(shape == shapeAns);
}

TEST_CASE_METHOD(CziCreator5,
                 "test_multiscene_mosaic_bboxes",
                 "[Reader_mosaic_bboxes]")
{
  auto czi = get();
  auto dSizes = czi->dimSizes();

  auto ans = czi->getAllSceneYXSize(0, true);
  assert(ans.size() == 4); // 2 channels * 44 m_index
  // 495643, 354924, 256, 256
  assert(ans[2].x == 495643);
  assert(ans[2].y == 354924);
  assert(ans[2].w == 256);
  assert(ans[2].h == 256);
}

#ifdef LOCAL_TEST

TEST_CASE_METHOD(CziCreatorBig, "test_big_czifile", "[Reader_timed_read]")
{
  auto czi = get();
  libCZI::CDimCoordinate dm =
    libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B, 0 },
                            { libCZI::DimensionIndex::S, 0 },
                            { libCZI::DimensionIndex::T, 1 },
                            { libCZI::DimensionIndex::Z, 5 } };

  std::stringstream info("Dims: ");
  info << czi->dimsString();
  INFO(info.str());
  auto dSizes = czi->dimSizes();

  std::stringstream dsizes("Shape: {");
  for_each(dSizes.begin(), dSizes.end(), [&dsizes](const int& x) {
    dsizes << x << ", ";
  });
  dsizes << "}" << std::endl;
  INFO(dsizes.str());

  auto start = std::chrono::high_resolution_clock::now();
  auto imgCont = czi->readSelected(dm, -1, CORES_FOR_THREADS);
  auto done = std::chrono::high_resolution_clock::now();

  std::cout << "Duration(milliseconds): "
            << std::chrono::duration_cast<std::chrono::milliseconds>(done -
                                                                     start)
                 .count();
  REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(done - start)
            .count() < 5050);
}

TEST_CASE_METHOD(CziCreatorBigM, "test_bigm_czifile", "[Reader_bbox]")
{
  auto czi = get();
  auto dSizes = czi->dimSizes();

  auto ans = czi->getAllSceneYXSize(0, true);
  assert(ans.size() == 88); // 2 channels * 44 m_index
  assert(ans[2].x == 22739);
  assert(ans[2].y == 19201);
  assert(ans[2].w == 950);
  assert(ans[2].h == 650);
}

#endif
