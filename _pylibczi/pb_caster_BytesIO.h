#ifndef _PYLIBCZI_PB_CASTER_BYTESIO_H
#define _PYLIBCZI_PB_CASTER_BYTESIO_H

#include <pybind11/pybind11.h>
#include <cstdio>
#include <iostream>
#include "CSimpleStreamImplFromFd.h"


namespace pybind11 {
  namespace detail {
    template<> struct type_caster< std::shared_ptr<libCZI::IStream> > {
    public:
        /**
         * This macro establishes the name 'FILE *' in
         * function signatures and declares a local variable
         * 'value' of type (FILE *)
         */
    PYBIND11_TYPE_CASTER(std::shared_ptr<libCZI::IStream>, _("IOBufferedReader"));

        /**
         * Conversion part 1 (Python->C++): convert a PyObject into a inty
         * instance or return false upon failure. The second argument
         * indicates whether implicit conversions should be applied.
         */
        bool load(handle src_, bool)
        {
            /* Extract PyObject from handle */
            PyObject* source = src_.ptr();

            /* Try converting into a Python integer value */
            int fDesc = PyObject_AsFileDescriptor(source);
            if (fDesc==-1) return false;
            value = std::shared_ptr<libCZI::IStream>(new pb_helpers::CSimpleStreamImplFromFd(fDesc));
            return (value!=nullptr && !PyErr_Occurred());
        }

        /**
         * Conversion part 2 (C++ -> Python): convert a FILE* instance into
         * a Python object. The second and third arguments are used to
         * indicate the return value policy and parent object (for
         * ``return_value_policy::reference_internal``) and are generally
         * ignored by implicit casters.
         */
        static handle cast(std::shared_ptr<libCZI::IStream> src_, return_value_policy /* policy */, handle /* parent */)
        {
            /*
             * FROM Python docs https://docs.python.org/3/c-api/file.html
             * Warning Since Python streams have their own buffering layer, mixing them with OS-level file
             * descriptors can produce various issues (such as unexpected ordering of data).
             * */
            std::cerr << "Conversion from libCZI::IStream to python file stream not defined" << std::endl;
            int fDesc = -1; // this is not implemented
            return PyFile_FromFd(fDesc, nullptr, "r", -1, nullptr, nullptr, nullptr, true); //true close fd on failure
        }
    };
  }
} // namespace pybind11::detail



#endif //_PYLIBCZI_PB_CASTER_BYTESIO_H
