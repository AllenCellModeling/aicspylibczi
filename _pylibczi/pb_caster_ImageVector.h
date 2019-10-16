#ifndef _PYLIBCZI_PB_CASTER_IMAGEVECTOR_H
#define _PYLIBCZI_PB_CASTER_IMAGEVECTOR_H

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "Image.h"
#include "pb_helpers.h"

namespace pybind11 {
  namespace detail {
    template<> struct type_caster<pylibczi::ImageVector> {
    public:
        /**
         * This macro establishes the name pylibczi::ImageVector  in
         * function signatures and declares a local variable
         * 'value' of type pylibczi::ImageVector
         */
    PYBIND11_TYPE_CASTER(pylibczi::ImageVector, _("numpy.ndarray"));

        /**
         * Conversion part 1 (Python->C++): convert a PyObject( numpy.ndarray ) into an ImageVector
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
         * Conversion part 2 (C++ -> Python): convert a pylibCZI::ImageVector instance into
         * a Python object, specifically a numpy.ndarray. The second and third arguments are used to
         * indicate the return value policy and parent object (for
         * ``return_value_policy::reference_internal``) and are generally
         * ignored by implicit casters.
         */
        static handle cast(pylibczi::ImageVector src_, return_value_policy /* policy */, handle /* parent */)
        {
            return pb_helpers::packArray(src_);
        }
    };
  }
} // namespace pybind11::detail


#endif //_PYLIBCZI_PB_CASTER_IMAGEVECTOR_H
