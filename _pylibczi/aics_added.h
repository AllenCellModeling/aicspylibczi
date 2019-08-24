//
// Created by Jamie Sherman on 7/11/19.
//

#ifndef PYLIBCZI_AICS_ADDED_HPP
#define PYLIBCZI_AICS_ADDED_HPP

#include <cstdio>
#include <iostream>
#include <typeinfo>
#include <exception>
#include <functional>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
//#include <numpy/ndarraytypes.h>

#include "inc_libCZI.h"
#include "Python.h"
#include "IndexMap.h"

using namespace std;

namespace py = pybind11;

// extern PyObject *PylibcziError; add back in when I split out into a cpp and hpp file
namespace pylibczi {

/// <summary>	A wrapper that takes a FILE * and creates an libCZI::IStream object out of it
class CSimpleStreamImplFromFP : public libCZI::IStream {
private:
  FILE *fp;
public:
  CSimpleStreamImplFromFP() = delete;
  explicit CSimpleStreamImplFromFP(FILE *file_pointer) : fp(file_pointer) {}
  ~CSimpleStreamImplFromFP() override { fclose(this->fp); };
  void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t *ptrBytesRead) override;
};

class Reader {
  unique_ptr<CCZIReader> m_czireader;
  libCZI::SubBlockStatistics m_statistics;
public:
  explicit Reader(FILE *f_in);

  typedef std::map<libCZI::DimensionIndex, std::pair<int, int> > mapDiP;

  bool isMosaicFile();

  Reader::mapDiP get_shape();

  typedef std::tuple<py::list, py::array_t<int32_t>, std::vector<IndexMap> > tuple_ans;
  // typedef py::list tuple_ans;

  tuple_ans cziread_selected(libCZI::CDimCoordinate &planeCoord, int mIndex = -1);

private:
  // int convertDictToPlaneCoords(PyObject *obj, void *dim_p);
  py::array copy_bitmap_to_numpy_array(std::shared_ptr<libCZI::IBitmapData> pBitmap);

  bool dimsMatch(const libCZI::CDimCoordinate &targetDims, const libCZI::CDimCoordinate &cziDims);

  void add_sort_order_index(vector<IndexMap> &vec);

  static bool isPyramid0(const libCZI::SubBlockInfo &info) {
      return (info.logicalRect.w == info.physicalSize.w && info.logicalRect.h == info.physicalSize.h);
  }
};


/*
/// @brief Function Prototypes

int convertDictToPlaneCoords(PyObject *obj, void *dim_p);

bool isPyramid0(const libCZI::SubBlockInfo &info);


std::shared_ptr<CSimpleStreamImplFromFP> cziread_io_buffered_reader_to_istream(PyObject *self, PyObject *args);

std::shared_ptr<libCZI::ICZIReader> open_czireader_from_istream(std::shared_ptr<CSimpleStreamImplFromFP> stream);

PyArrayObject *copy_bitmap_to_numpy_array(std::shared_ptr<libCZI::IBitmapData> pBitmap);

PyObject *get_shape_from_fp(std::shared_ptr<libCZI::ICZIReader> &czi);

PyObject *cziread_shape_from_istream(PyObject *self, PyObject *args);

PyObject *cziread_selected(PyObject *self, PyObject *args);

void add_sort_order_index(vector<IndexMap> &vec);

/// <summary>	A wrapper that takes a FILE * and creates an libCZI::IStream object out of it
//class CSimpleStreamImplFromFP : public libCZI::IStream {
//private:
//  FILE *fp;
//public:
//  CSimpleStreamImplFromFP() = delete;
//
//  explicit CSimpleStreamImplFromFP(FILE *file_pointer) : fp(file_pointer) {}
//
//  ~CSimpleStreamImplFromFP() override = default;
//
//public:    // interface libCZI::IStream
//  void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t *ptrBytesRead) override {
//    fseeko(this->fp, offset, SEEK_SET);
//
//    std::uint64_t bytesRead = fread(pv, 1, (size_t) size, this->fp);
//    if (ptrBytesRead != nullptr) {
//      *ptrBytesRead = bytesRead;
//    }
//  }
//};

/// <summary> Custom Exception class for when there is a problem converting the PyObject * to a
/// FileDescriptor to a file pointer
class BadFileDescriptorException : public std::exception {
  const char *what() const throw() {
    return "Couldn't Convert Python File stream to FILE *";
  }
};

/// <summary> Custom Exception class for arguments that can't be converted from PyObject * to C++ objects
class BadArgsException : public std::exception {
  const char *what() const throw() {
    return "Couldn't Convert Python Arguments to C++ objects";
  }
};

int convertDictToPlaneCoords(PyObject *obj, void *dim_p) {
  if (!PyDict_Check(obj)) { // docs says it returns true/false but it returns an integer
    return 0; // not a dictionary somethings wrong
  };

  auto *dims = static_cast<libCZI::CDimCoordinate *>(dim_p);
  std::map<std::string, libCZI::DimensionIndex> tbl{
      {"V", libCZI::DimensionIndex::V},
      {"H", libCZI::DimensionIndex::H},
      {"I", libCZI::DimensionIndex::I},
      {"S", libCZI::DimensionIndex::S},
      {"R", libCZI::DimensionIndex::R},
      {"T", libCZI::DimensionIndex::T},
      {"C", libCZI::DimensionIndex::C},
      {"Z", libCZI::DimensionIndex::Z}
  };

  int ret_val = 1;
  std::for_each(tbl.begin(), tbl.end(), [&](std::pair<const std::string, libCZI::DimensionIndex> &kv) -> void {
    PyObject *pyInt = PyDict_GetItemString(obj, kv.first.c_str());
    if (pyInt != NULL) {
      dims->Set(kv.second, static_cast<int>(PyLong_AsLong(pyInt)));
      if (PyErr_Occurred() != NULL) {
        PyErr_SetString(PylibcziError,
                        "problem converting Dictionary of dims, should be C=1 meaning Dimension = Integer");
        ret_val = 0;
      }
    }
  });
  return ret_val; // success
}

/// Read the metadata from the python file stream and return it as a string
/// \param self PyObject * to the object that called this code -- not used here but being consistent with convention
/// \param pyfp The python PyObject BytesIO object or relative there of the opened file
/// \return A string containing the XML metadata
static PyObject *cziread_meta_from_istream(PyObject *self, PyObject *pyfp) {
  PyObject *in_file = nullptr;
  if (!PyArg_ParseTuple(pyfp, "O", &in_file)) {
    PyErr_SetString(PylibcziError, "Error: conversion of arguments failed. Check arguments.");
    return nullptr;
  }
  auto stream = cziread_io_buffered_reader_to_istream(self, in_file);
  auto cziReader = open_czireader_from_istream(stream);

  // get the the document's metadata
  auto mds = cziReader->ReadMetadataSegment();
  auto md = mds->CreateMetaFromMetadataSegment();
  //auto docInfo = md->GetDocumentInfo();
  //auto dsplSettings = docInfo->GetDisplaySettings();
  std::string xml = md->GetXml();
  // copy the metadata into python string
  PyObject *pystring = Py_BuildValue("s", xml.c_str());

  cziReader->Close();
  return pystring;
}

/// @brief get_shape_from_fp returns the Dimensions of a ZISRAW/CZI when provided a ICZIReader object
/// \param czi: a shared_ptr to an initialized CziReader object
/// \return A Python Dictionary as a PyObject*
PyObject *get_shape_from_fp(std::shared_ptr<libCZI::ICZIReader> &czi) {
  PyObject *ans = nullptr;
  auto statistics = czi->GetStatistics();
  std::map<libCZI::DimensionIndex, std::pair<int, int> > tbl;

  statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di, int start, int size) -> bool {
    tbl.emplace(di, std::make_pair(start, size));
    return true;
  });

  PyObject *pyDict = PyDict_New();
  std::for_each(tbl.begin(), tbl.end(),
                [&pyDict](const std::pair<libCZI::DimensionIndex, std::pair<int, int> > &pare) {
                  std::string tmp(1, libCZI::Utils::DimensionToChar(pare.first));
                  PyObject *key = Py_BuildValue("s", tmp.c_str());
                  PyObject *value = Py_BuildValue("i", (pare.second.second));
                  PyDict_SetItem(pyDict, key, value);
                  Py_DECREF(key);
                  Py_DECREF(value);
                });
  ans = pyDict;
  return ans;
}

/// This function returns the Dimensions of the ZISRAW/CZI file
/// in some cases if a channel's depth is 1 it is not recorded. It is unclear if this is caused by
/// acquisition settings or writer default behavior.
///
/// Example:
///      with open(data_dir/fname, 'rb') as fp:
///          czi = CziFile(czi_filename=fp)
///          shape = czi.dims()
///
/// \param self: The reference to the python object calling the library
/// \param pyfp: an open file stream
/// \return a dictionary with {'Channel': Depth} -> {'S': 5, 'C':2} would be a 5 Scene 2 Channel image
PyObject *cziread_shape_from_istream(PyObject *self, PyObject *pyfp) {
  PyObject *ans = nullptr;
  PyObject *in_file = nullptr;
  try {
    if (!PyArg_ParseTuple(pyfp, "O", &in_file)) {
      PyErr_SetString(PylibcziError, "Error: conversion of arguments failed. Check arguments.");
      return nullptr;
    }
    auto stream = cziread_io_buffered_reader_to_istream(self, in_file);
    auto cziReader = open_czireader_from_istream(stream);
    ans = get_shape_from_fp(cziReader);
  } catch (const exception &e) {
    PyErr_SetString(PyExc_IOError, "Unable to lookup Dimensions in File.");
  }
  return ans;
}

static PyObject *cziread_allsubblocks_from_istream(PyObject *self, PyObject *pyfp) {
  using namespace std::placeholders; // enable _1 _2 _3 type placeholders
  PyObject *in_file = nullptr;
  // parse arguments
  try {
    if (!PyArg_ParseTuple(pyfp, "O", &in_file)) {
      PyErr_SetString(PylibcziError, "Error: conversion of arguments failed. Check arguments.");
      return nullptr;
    }
    auto stream = cziread_io_buffered_reader_to_istream(self, in_file);
    auto cziReader = open_czireader_from_istream(stream);
    // count all the subblocks

    npy_intp subblock_count = 0;
    auto count_blocks([&subblock_count](int idx, const libCZI::SubBlockInfo &info) -> bool {
      subblock_count++;
      return true;
    });

    // assignment warning is a CLION error it should be fine.
    std::function<bool(int, const libCZI::SubBlockInfo &)> countLambdaAsFunc =
        static_cast< std::function<bool(int, const libCZI::SubBlockInfo &)> >(count_blocks);

    cziReader->EnumerateSubBlocks(countLambdaAsFunc);  // f_count_blocks);
    std::cout << "Enumerated " << subblock_count << std::endl;

    // meh - this seems to be not useful, what is an M-index? someone read the spec...
    //auto stats = cziReader->GetStatistics();
    //cout << stats.subBlockCount << " " << stats.maxMindex << endl;
    //int subblock_count = stats.subBlockCount;

    // copy the image data and coordinates into numpy arrays, return images as python list of numpy arrays
    PyObject *images = PyList_New(subblock_count);
    npy_intp eshp[2];
    eshp[0] = subblock_count;
    eshp[1] = 2;
    PyArrayObject *coordinates = (PyArrayObject *) PyArray_Empty(2, eshp, PyArray_DescrFromType(NPY_INT32), 0);
    npy_int32 *coords = (npy_int32 *) PyArray_DATA(coordinates);

    npy_intp cnt = 0;
    cziReader->EnumerateSubBlocks(
        [&cziReader, &subblock_count, &cnt, images, coords](int idx, const libCZI::SubBlockInfo &info) {
          //std::cout << "Index " << idx << ": " << libCZI::Utils::DimCoordinateToString(&info.coordinate)
          //  << " Rect=" << info.logicalRect << " M-index " << info.mIndex << std::endl;

          // add the sub-block image
          PyList_SetItem(images, cnt,
                         (PyObject *) copy_bitmap_to_numpy_array(
                             cziReader->ReadSubBlock(idx)->CreateBitmap()));
          // add the coordinates
          coords[2 * cnt] = info.logicalRect.x;
          coords[2 * cnt + 1] = info.logicalRect.y;

          //info.coordinate.EnumValidDimensions([](libCZI::DimensionIndex dim, int value)
          //{
          //    //valid_dims[(int) dim] = true;
          //    cout << "Dimension  " << dim << " value " << value << endl;
          //    return true;
          //});

          cnt++;
          return true;
        });

    return Py_BuildValue("OO", images, (PyObject *) coordinates);
  }
  catch (const BadArgsException &e) {
    PyErr_SetString(PyExc_TypeError, "Unable to map args provided from python to c++.");
    return NULL;
  }
  catch (const BadFileDescriptorException &fbad) {
    PyErr_SetString(PyExc_IOError, "Unable to convert ByteIO object to File pointer.");
    return NULL;
  }
  return NULL;
}

bool isPyramid0(const libCZI::SubBlockInfo &info) {
  return (info.logicalRect.w == info.physicalSize.w && info.logicalRect.h == info.physicalSize.h);
}

bool dimsMatch(const libCZI::CDimCoordinate &targetDims, const libCZI::CDimCoordinate &cziDims) {
  bool ans = true;
  targetDims.EnumValidDimensions([&](libCZI::DimensionIndex dim, int value) -> bool {
    int cziDimValue = 0;
    if (cziDims.TryGetPosition(dim, &cziDimValue)) {
      ans = (cziDimValue == value);
    }
    return ans;
  });
  return ans;
}

PyObject *cziread_selected(PyObject *self, PyObject *args) {
  // (fp, dict, mIndex)
  PyObject *in_file = nullptr;
  PyObject *pyMIndex = nullptr;
  libCZI::CDimCoordinate planeCoord;
  if (!PyArg_ParseTuple(args, "OO&O", &in_file, &convertDictToPlaneCoords, &planeCoord, &pyMIndex)) {
    PyErr_SetString(PylibcziError, "Error: conversion of arguments failed. Check arguments.");
    return nullptr;
  }
  auto stream = cziread_io_buffered_reader_to_istream(self, in_file);
  auto cziReader = open_czireader_from_istream(stream);

  // convert mIndex
  int mIndex = -1;
  if (pyMIndex != Py_None)
    mIndex = static_cast<int>(PyLong_AsLong(pyMIndex));

  // count the matching subblocks
  npy_intp matching_subblock_count = 0;
  std::vector<IndexMap> order_mapping;
  cziReader->EnumerateSubBlocks([&](int idx, const libCZI::SubBlockInfo &info) -> bool {
    if (isPyramid0(info) && dimsMatch(planeCoord, info.coordinate)) {
      order_mapping.emplace_back(idx, info.coordinate);
      matching_subblock_count++;
    }
    return true;
  });

  add_sort_order_index(order_mapping);

  auto statistics = cziReader->GetStatistics();

  // get scene index if specified
  int scene_index = -1;
  libCZI::IntRect sceneBox = {0, 0, -1, -1};
  if (planeCoord.TryGetPosition(libCZI::DimensionIndex::S, &scene_index)) {
    auto itt = statistics.sceneBoundingBoxes.find(scene_index);
    if (itt == statistics.sceneBoundingBoxes.end())
      sceneBox = itt->second.boundingBoxLayer0; // layer0 specific
    else
      sceneBox.Invalidate();
  } else {
    std::cout << "You are attempting to extract a scene from a single scene czi." << std::endl;
    scene_index = -1;
  }

  PyObject *images = PyList_New(matching_subblock_count);
  npy_intp eshp[2];
  eshp[0] = matching_subblock_count;
  eshp[1] = 2;
  PyArrayObject *coordinates = (PyArrayObject *) PyArray_Empty(2, eshp, PyArray_DescrFromType(NPY_INT32), 0);
  npy_int32 *coords = (npy_int32 *) PyArray_DATA(coordinates);

  npy_intp cnt = 0;
  cziReader->EnumerateSubBlocks([&](int idx, const libCZI::SubBlockInfo &info) {

    if (!isPyramid0(info))
      return true;
    if (sceneBox.IsValid() && !sceneBox.IntersectsWith(info.logicalRect))
      return true;
    if (!dimsMatch(planeCoord, info.coordinate))
      return true;
    if (mIndex != -1 && info.mIndex != std::numeric_limits<int>::max() && mIndex != info.mIndex)
      return true;

    // add the sub-block image
    PyList_SetItem(images, cnt,
                   (PyObject *) copy_bitmap_to_numpy_array(cziReader->ReadSubBlock(idx)->CreateBitmap()));
    // add the coordinates
    coords[2 * cnt] = info.logicalRect.x;
    coords[2 * cnt + 1] = info.logicalRect.y;

    //info.coordinate.EnumValidDimensions([](libCZI::DimensionIndex dim, int value)
    //{
    //    //valid_dims[(int) dim] = true;
    //    cout << "Dimension  " << dim << " value " << value << endl;
    //    return true;
    //});

    cnt++;
    return true;
  });

  return Py_BuildValue("OO", images, (PyObject *) coordinates);
}

void add_sort_order_index(vector<IndexMap> &vec) {
  int counter = 0;
  std::sort(vec.begin(), vec.end(), [](IndexMap &a, IndexMap &b) -> bool { return (a < b); });
  for (auto &&a : vec)
    a.m_position = counter++;
  std::sort(vec.begin(), vec.end(),
            [](IndexMap &a, IndexMap &b) -> bool { return (a.m_subblockIndex < b.m_subblockIndex) };);
}

/// @brief Convert the python BytesIO / IOBufferedReader object to a child of libCZI::IStream
/// \param self PyObject * to the object that made the call -- not used here
/// \param pyfp PyObject * to the IOBufferedReader / BytesIO python stream of file contents
/// \return a shared_ptr to a CSimpleStreamImplFromFp -- a wrapper of a file pointer that is compatible with libCZI
std::shared_ptr<CSimpleStreamImplFromFP> cziread_io_buffered_reader_to_istream(PyObject *self, PyObject *in_file) {
  // parse arguments
  int fdescript = PyObject_AsFileDescriptor(in_file);
  if (fdescript == -1)
    throw BadFileDescriptorException();
  FILE *fp = fdopen(fdescript, "r");
  if (fp == nullptr)
    throw BadFileDescriptorException();
  return std::make_shared<CSimpleStreamImplFromFP>(fp);
}

/// @brief This function constructs and returns a CziReader class as a shared_ptr.
/// \param stream A CSimpleImplFromFP object say from \related cziread_io_buffered_reader_to_istream
/// \return a shared pointer to an initialized CziReader
std::shared_ptr<libCZI::ICZIReader> open_czireader_from_istream(std::shared_ptr<CSimpleStreamImplFromFP> stream) {
  // open the czi file
  auto cziReader = libCZI::CreateCZIReader();
  cziReader->Open(stream);
  return cziReader;
}
 */

}

#endif //PYLIBCZI_AICS_ADDED_HPP
