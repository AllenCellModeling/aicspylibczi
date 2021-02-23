#ifndef _PYLIBCZI_PB_CASTER_IMAGESCONTAINER_H
#define _PYLIBCZI_PB_CASTER_IMAGESCONTAINER_H

#include "ImagesContainer.h"
#include "pb_helpers.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace pybind11 {
namespace detail {
template<>
struct type_caster<pylibczi::ImagesContainerBase::ImagesContainerBasePtr>
{
public:
  /**
   * This macro establishes the name pylibczi::ImageContainerBasePtr in
   * function signatures and declares a local variable
   * 'value' of type pylibczi::ImageContainerBasePtr
   */
  PYBIND11_TYPE_CASTER(pylibczi::ImagesContainerBase::ImagesContainerBasePtr, _("numpy.ndarray"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject( numpy.ndarray ) into an ImageContainerBasePtr
   * instance or return false upon failure. The second argument
   * indicates whether implicit conversions should be applied.
   */
  bool load(handle src_, bool)
  {
    // Currently not used, if casting a numpy.ndarray to an ImagesContainer is required this must be implemented=

    /* Extract PyObject from handle */
    PyObject* source = src_.ptr();
    return (false); // no conversion is done here so if this code is called always fail
  }

  /**
   * Conversion part 2 (C++ -> Python): convert a pylibCZI::ImagesContainer instance into
   * a Python object, specifically a numpy.ndarray. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(pylibczi::ImagesContainerBase::ImagesContainerBasePtr src_,
                     return_value_policy /* policy */,
                     handle /* parent */)
  {
    return pb_helpers::packArray(
      src_); // the helper takes the ImagesContainerBasePtr and converts it to a numpy ndarray
  }
};
}
} // namespace pybind11::detail

#endif //_PYLIBCZI_PB_CASTER_IMAGEVECTOR_H
