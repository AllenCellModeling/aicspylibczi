#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "IndexMap.h"
#include "Reader.h"
#include "exceptions.h"
#include "inc_libCZI.h"

// the below headers are crucial otherwise the custom casts aren't recognized
#include "pb_caster_BytesIO.h"
#include "pb_caster_DimIndex.h"
#include "pb_caster_ImagesContainer.h"
#include "pb_caster_SubblockMetaVec.h"
#include "pb_caster_libCZI_DimensionIndex.h"

PYBIND11_MODULE(_aicspylibczi, m)
{

  namespace py = pybind11;

  m.doc() =
    "aicspylibczi C++ extension for reading ZISRAW/CZI files"; // optional
                                                               // module
                                                               // docstring

  py::register_exception<pylibczi::FilePtrException>(
    m, "PylibCZI_BytesIO2FilePtrException");
  py::register_exception<pylibczi::PixelTypeException>(
    m, "PylibCZI_PixelTypeException");
  py::register_exception<pylibczi::RegionSelectionException>(
    m, "PylibCZI_RegionSelectionException");
  py::register_exception<pylibczi::ImageAccessUnderspecifiedException>(
    m, "PylibCZI_ImageAccessUnderspecifiedException");
  py::register_exception<pylibczi::ImageIteratorException>(
    m, "PylibCZI_ImageIteratorException");
  py::register_exception<pylibczi::ImageSplitChannelException>(
    m, "PylibCZI_ImageSplitChannelException");
  py::register_exception<pylibczi::ImageCopyAllocFailed>(
    m, "PylibCZI_ImageCopyAllocFailed");
  py::register_exception<pylibczi::CdimSelectionZeroImagesException>(
    m, "PylibCZI_CDimSpecSelectedNoImagesException");
  py::register_exception<pylibczi::CDimCoordinatesOverspecifiedException>(
    m, "PylibCZI_CDimCoordinatesOverspecifiedException");
  py::register_exception<pylibczi::CDimCoordinatesUnderspecifiedException>(
    m, "PylibCZI_CDimCoordinatesUnderspecifiedException");

  py::class_<pylibczi::Reader>(m, "Reader")
    .def(py::init<std::shared_ptr<libCZI::IStream>>())
    .def("is_mosaic", &pylibczi::Reader::isMosaic)
    .def("has_consistent_shape", &pylibczi::Reader::shapeIsConsistent)
    .def("read_dims", &pylibczi::Reader::readDimsRange)
    .def("read_dims_string", &pylibczi::Reader::dimsString)
    .def("read_dims_sizes", &pylibczi::Reader::dimSizes)
    .def("read_scene_wh", &pylibczi::Reader::getSceneYXSize)
    .def("read_mosaic_scene_boxes", &pylibczi::Reader::getAllSceneYXSize)
    .def("read_meta", &pylibczi::Reader::readMeta)
    .def("read_selected", &pylibczi::Reader::readSelected)
    .def("mosaic_shape", &pylibczi::Reader::mosaicShape)
    .def("read_meta_from_subblock", &pylibczi::Reader::readSubblockMeta)
    .def("read_rect_from_subblock", &pylibczi::Reader::readSubblockRect)
    .def("read_mosaic", &pylibczi::Reader::readMosaic)
    .def("pixel_type", &pylibczi::Reader::pixelType);

  py::class_<pylibczi::IndexMap>(m, "IndexMap")
    .def(py::init<>())
    .def("is_m_index_valid", &pylibczi::IndexMap::isMIndexValid)
    .def("dim_index", &pylibczi::IndexMap::dimIndex)
    .def("m_index", &pylibczi::IndexMap::mIndex);

  py::class_<libCZI::CDimCoordinate>(m, "DimCoord")
    .def(py::init<>())
    .def("set_dim", &libCZI::CDimCoordinate::Set);

  py::class_<libCZI::IntRect>(m, "IntRect")
    .def(py::init<>())
    .def_readwrite("x", &libCZI::IntRect::x)
    .def_readwrite("y", &libCZI::IntRect::y)
    .def_readwrite("w", &libCZI::IntRect::w)
    .def_readwrite("h", &libCZI::IntRect::h);
}
