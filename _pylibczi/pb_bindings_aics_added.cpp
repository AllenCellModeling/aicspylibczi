//
// Created by Jamie Sherman on 2019-08-20.
//

#include <pybind11/pybind11.h>
#include "inc_libCZI.h"
#include "aics_added.h"
#include "IndexMap.h"


PYBIND11_MODULE(_pylibczi, m) {

    namespace py = pybind11;

    /*
     * 		Z = 1,			///< The Z-dimension.
		C = 2,			///< The C-dimension ("channel").
		T = 3,			///< The T-dimension ("time").
		R = 4,			///< The R-dimension ("rotation").
		S = 5,			///< The S-dimension ("scene").
		I = 6,			///< The I-dimension ("illumination").
		H = 7,			///< The H-dimension ("phase").
		V = 8,			///< The V-dimension ("view").
		B = 9,			///< The B-dimension ("block") - its use is deprecated.
     *
    // could add this in but I don't think I need it, it provides a wrapper for the C++ object in python rather
     than casting it from one side to the other. 
    py::enum_<libCZI::DimensionIndex>(m, "DimIndex")
        .value("invalid", libCZI::DimensionIndex::invalid)
        .value("Z", libCZI::DimensionIndex::Z)
        .value("C", libCZI::DimensionIndex::C)
        .value("T", libCZI::DimensionIndex::T)
        .value("R", libCZI::DimensionIndex::R)
        .value("S", libCZI::DimensionIndex::S)
        .value("I", libCZI::DimensionIndex::I)
        .value("H", libCZI::DimensionIndex::H)
        .value("V", libCZI::DimensionIndex::V)
        .value("B", libCZI::DimensionIndex::B)
        .export_values();

     */
    py::class_<pylibczi::Reader>(m, "Reader")
        .def(py::init<FILE *>())
        .def("is_mosaic_file", &pylibczi::Reader::isMosaicFile)
        .def("get_shape", &pylibczi::Reader::get_shape)
        .def("read_selected", &pylibczi::Reader::cziread_selected);

    py::class_<pylibczi::IndexMap>(m, "IndexMap")
        .def(py::init<>())
        .def("is_m_index_valid", &pylibczi::IndexMap::IsMIndexValid)
        .def("dim_index", &pylibczi::IndexMap::dimIndex)
        .def("m_index", &pylibczi::IndexMap::mIndex);

}