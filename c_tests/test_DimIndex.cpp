//
#include "catch.hpp"
#include "inc_libCZI.h"
#include <iostream>

#include "DimIndex.h"
#include "helper_algorithms.h"

using namespace pylibczi;

TEST_CASE("constants_dimension", "[constants_DimMap]")
{
  using DMI = DimIndex;
  using DI = libCZI::DimensionIndex;
  std::vector<DimIndex> input = { DMI::B, DMI::V, DMI::H, DMI::I, DMI::S, DMI::R,
                                  DMI::T, DMI::C, DMI::Z, DMI::Y, DMI::X, DMI::A };
  std::vector<DI> output = { DI::B, DI::V, DI::H, DI::I,       DI::S,       DI::R,
                             DI::T, DI::C, DI::Z, DI::invalid, DI::invalid, DI::invalid };
  pairedForEach(input.begin(), input.end(), output.begin(), [](DMI in_, DI expected_) {
    REQUIRE(dimIndexToDimensionIndex(in_) == expected_);
  });
}
