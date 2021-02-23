#include "pylibczi_ostream.h"

ostream&
operator<<(ostream& out_, const libCZI::CDimCoordinate& plane_coordante_)
{
  stringstream tmp;
  plane_coordante_.EnumValidDimensions([&tmp](libCZI::DimensionIndex di_, int val_) {
    tmp << (tmp.str().empty() ? "CDimCoordinate: {" : ", ");
    tmp << libCZI::Utils::DimensionToChar(di_) << ": " << val_;
    return true;
  });
  tmp << "}";
  out_ << tmp.str();
  return out_;
}

ostream&
operator<<(ostream& out_, const libCZI::CDimBounds& plane_coordinate_bounds_)
{
  stringstream tmp;
  plane_coordinate_bounds_.EnumValidDimensions([&tmp](libCZI::DimensionIndex di_, int start_, int length_) {
    tmp << (tmp.str().empty() ? "CDimBounds: {" : ", ");
    tmp << libCZI::Utils::DimensionToChar(di_) << ": (" << start_ << "," << length_ << ")";
    return true;
  });
  tmp << "}";
  out_ << tmp.str();

  return out_;
}
