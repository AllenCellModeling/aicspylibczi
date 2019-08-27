//
// Created by Jamie Sherman on 2019-08-20.
//

#include <pybind11/pybind11.h>
#include "inc_libCZI.h"
#include "Reader.h"
#include "IndexMap.h"
#include "exceptions.h"


PYBIND11_MODULE(_pylibczi, m) {

    namespace py = pybind11;

    py::register_exception<pylibczi::FilePtrException>(m, "PyBytesIO2FilePtrException");
    py::register_exception<pylibczi::PixelTypeException>(m, "PyPixelTypeException");

    py::class_<pylibczi::Reader>(m, "Reader")
        .def(py::init<FILE *>())
        .def("is_mosaic_file", &pylibczi::Reader::isMosaicFile)
        .def("get_shape", &pylibczi::Reader::get_shape)
        .def("read_meta", &pylibczi::Reader::cziread_meta)
        .def("read_selected", &pylibczi::Reader::cziread_selected);

    py::class_<pylibczi::IndexMap>(m, "IndexMap")
        .def(py::init<>())
        .def("is_m_index_valid", &pylibczi::IndexMap::IsMIndexValid)
        .def("dim_index", &pylibczi::IndexMap::dimIndex)
        .def("m_index", &pylibczi::IndexMap::mIndex);

}