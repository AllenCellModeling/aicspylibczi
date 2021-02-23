#ifndef _PYLIBCZI_PB_CASTER_SUBBLOCKMETAVEC_H
#define _PYLIBCZI_PB_CASTER_SUBBLOCKMETAVEC_H

#include "SubblockMetaVec.h"
#include "pb_helpers.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace pybind11 {
namespace detail {
template<>
struct type_caster<pylibczi::SubblockMetaVec>
{
public:
  /**
   * This macro establishes the name pylibczi::SubblockMetaVec  in
   * function signatures and declares a local variable
   * 'value' of type pylibczi::SubblockMetaVec
   */
  PYBIND11_TYPE_CASTER(pylibczi::SubblockMetaVec, _("List"));

  /**
   * Conversion part 1 (Python->C++): convert a PyObject( numpy.ndarray ) into an SubblockMetaVec
   * instance or return false upon failure. The second argument
   * indicates whether implicit conversions should be applied.
   */
  bool load(handle src_, bool)
  {
    // Currently not used, if casting a numpy.ndarray to an ImageVector is required this must be implemented=

    /* Extract PyObject from handle */
    PyObject* source = src_.ptr();
    return (false); // no conversion is done here so if this code is called always fail
  }

  /**
   * Conversion part 2 (C++ -> Python): convert a pylibCZI::SubblockMetaVec instance into
   * a Python object, specifically a numpy.ndarray. The second and third arguments are used to
   * indicate the return value policy and parent object (for
   * ``return_value_policy::reference_internal``) and are generally
   * ignored by implicit casters.
   */
  static handle cast(pylibczi::SubblockMetaVec src_, return_value_policy /* policy */, handle /* parent */)
  {
    py::list* l = pb_helpers::packStringArray(src_);
    return *l;
  }
};
}
}
#endif //_PYLIBCZI_PB_CASTER_SUBBLOCKMETAVEC_H
