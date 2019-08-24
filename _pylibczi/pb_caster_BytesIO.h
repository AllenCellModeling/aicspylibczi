//
// Created by Jamie Sherman on 2019-08-20.
//

#ifndef _PYLIBCZI__PYLIBCZI_PB_BYTESIO_CASTER_H
#define _PYLIBCZI__PYLIBCZI_PB_BYTESIO_CASTER_H

#include <pybind11/pybind11.h>
#include <cstdio>

// TODO Make windows compatible (this is very unix centric)
namespace pybind11 {
    namespace detail {
        template<> struct type_caster<FILE *> {
        public:
          /**
           * This macro establishes the name 'FILE *' in
           * function signatures and declares a local variable
           * 'value' of type (FILE *)
           */
          PYBIND11_TYPE_CASTER(FILE *, _("BytesIO"));

          /**
           * Conversion part 1 (Python->C++): convert a PyObject into a inty
           * instance or return false upon failure. The second argument
           * indicates whether implicit conversions should be applied.
           */
          bool load(handle src, bool) {
              /* Extract PyObject from handle */
              PyObject *source = src.ptr();
              /* Try converting into a Python integer value */
              int f_desc = PyObject_AsFileDescriptor(source);
              if(f_desc == -1 ) return false;
              value = fdopen(f_desc, "r");
              return (value != nullptr && !PyErr_Occurred());
          }

          /**
           * Conversion part 2 (C++ -> Python): convert a FILE* instance into
           * a Python object. The second and third arguments are used to
           * indicate the return value policy and parent object (for
           * ``return_value_policy::reference_internal``) and are generally
           * ignored by implicit casters.
           */
          static handle cast(FILE *src, return_value_policy /* policy */, handle /* parent */) {
              /*
               * FROM Python docs https://docs.python.org/3/c-api/file.html
               * Warning Since Python streams have their own buffering layer, mixing them with OS-level file
               * descriptors can produce various issues (such as unexpected ordering of data).
               * */
              int f_desc = fileno(src);
              return PyFile_FromFd(f_desc, nullptr, "r", -1, nullptr, nullptr, nullptr, true); //true close fd on failure
          }
        };
    }
} // namespace pybind11::detail



#endif //_PYLIBCZI__PYLIBCZI_PB_BYTESIO_CASTER_H
