#ifndef _PYLIBCZI_PB_CASTER_LIBCZI_DIMENSIONINDEX_H
#define _PYLIBCZI_PB_CASTER_LIBCZI_DIMENSIONINDEX_H

#include <pybind11/pybind11.h>

#include "inc_libCZI.h"

namespace pybind11 {
namespace detail {
template<>
struct type_caster<libCZI::DimensionIndex>
{
public:
  /**
   * This macro establishes the name libCZI::DimensionIndex  in
   * function signatures and declares a local variable
   * 'value' of type libCZI::DimensionIndex
   */
  PYBIND11_TYPE_CASTER(libCZI::DimensionIndex, _("str"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject into a libCZI::DimensionIndex
   * instance or return false upon failure. The second argument
   * indicates whether implicit conversions should be applied.
   */
  bool load(handle src_, bool)
  {
    /* Extract PyObject from handle */
    PyObject* source = src_.ptr();
    const char* letterPtr = PyUnicode_AsUTF8(source);
    if (letterPtr == nullptr)
      return false;
    char letter = letterPtr[0];
    value = libCZI::Utils::CharToDimension(letter);
    return (value != libCZI::DimensionIndex::invalid && !PyErr_Occurred());
  }

  /**
   * Conversion part 2 (C++ -> Python): convert a FILE* instance into
   * a Python object. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(libCZI::DimensionIndex src_, return_value_policy /* policy */, handle /* parent */)
  {
    /*
     * FROM Python docs https://docs.python.org/3/c-api/file.html
     * Warning Since Python streams have their own buffering layer, mixing them with OS-level file
     * descriptors can produce various issues (such as unexpected ordering of data).
     * */
    std::string tmp(1, libCZI::Utils::DimensionToChar(src_));
    return PyUnicode_FromString(tmp.c_str());
  }
};
}
} // namespace pybind11::detail

#endif //_PYLIBCZI_PB_CASTER_LIBCZI_DIMENSIONINDEX_H
