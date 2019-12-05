#include "constants.h"
#include "inc_libCZI.h"

namespace pylibczi {

  const std::initializer_list<Constants::CziDi>
  Constants::s_sortOrder{Constants::CziDi::B,
                         Constants::CziDi::V,
                         Constants::CziDi::H,
                         Constants::CziDi::I,
                         Constants::CziDi::S,
                         Constants::CziDi::R,
                         Constants::CziDi::T,
                         Constants::CziDi::C,
                         Constants::CziDi::Z
  };


  bool
  Constants::dimsMatch(const libCZI::CDimCoordinate& target_dims_, const libCZI::CDimCoordinate& czi_dims_)
  {
      bool ans = true;
      target_dims_.EnumValidDimensions([&](libCZI::DimensionIndex dim_, int value_) -> bool {
          int cziDimValue = 0;
          if (czi_dims_.TryGetPosition(dim_, &cziDimValue)) {
              ans = (cziDimValue==value_);
          }
          return ans;
      });
      return ans;
  }

}
