#include "DimIndex.h"

namespace pylibczi {
char
dimIndexToChar(DimIndex index_)
{
  switch (index_) {
    case DimIndex::A:
      return 'A';
    case DimIndex::X:
      return 'X';
    case DimIndex::Y:
      return 'Y';
    case DimIndex::Z:
      return 'Z';
    case DimIndex::C:
      return 'C';
    case DimIndex::T:
      return 'T';
    case DimIndex::R:
      return 'R';
    case DimIndex::M:
      return 'M';
    case DimIndex::S:
      return 'S';
    case DimIndex::I:
      return 'I';
    case DimIndex::H:
      return 'H';
    case DimIndex::V:
      return 'V';
    case DimIndex::B:
      return 'B';
    default:
      return '?';
  }
}

DimIndex
charToDimIndex(char c_)
{
  switch (c_) {
    case 'a':
    case 'A':
      return DimIndex::A;
    case 'x':
    case 'X':
      return DimIndex::X;
    case 'y':
    case 'Y':
      return DimIndex::Y;
    case 'z':
    case 'Z':
      return DimIndex::Z;
    case 'c':
    case 'C':
      return DimIndex::C;
    case 't':
    case 'T':
      return DimIndex::T;
    case 'r':
    case 'R':
      return DimIndex::R;
    case 'm':
    case 'M':
      return DimIndex::M;
    case 's':
    case 'S':
      return DimIndex::S;
    case 'i':
    case 'I':
      return DimIndex::I;
    case 'h':
    case 'H':
      return DimIndex::H;
    case 'v':
    case 'V':
      return DimIndex::V;
    case 'b':
    case 'B':
      return DimIndex::B;
    default:
      return DimIndex::invalid;
  }
}

libCZI::DimensionIndex
dimIndexToDimensionIndex(DimIndex dim_index_)
{
  return libCZI::Utils::CharToDimension(dimIndexToChar(dim_index_));
}

DimIndex
dimensionIndexToDimIndex(libCZI::DimensionIndex index_)
{
  return charToDimIndex(libCZI::Utils::DimensionToChar(index_));
}

}
