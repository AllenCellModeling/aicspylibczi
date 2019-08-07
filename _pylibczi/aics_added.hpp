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

#include "inc_libCZI.h"
#include "Python.h"

using namespace std;


/// @brief Function Prototypes
class CSimpleStreamImplFromFP;
std::shared_ptr <CSimpleStreamImplFromFP> cziread_io_buffered_reader_to_istream(PyObject *self, PyObject *args);
std::shared_ptr <libCZI::ICZIReader> open_czireader_from_istream(std::shared_ptr <CSimpleStreamImplFromFP> stream);
PyArrayObject *copy_bitmap_to_numpy_array(std::shared_ptr <libCZI::IBitmapData> pBitmap);
PyObject *get_shape_from_fp(std::shared_ptr <libCZI::ICZIReader> &czi);
PyObject *cziread_shape_from_istream(PyObject *self, PyObject *args);


/// <summary>	A wrapper that takes a FILE * and creates an libCZI::IStream object out of it
class CSimpleStreamImplFromFP : public libCZI::IStream {
private:
    FILE *fp;
public:
    CSimpleStreamImplFromFP() = delete;

    explicit CSimpleStreamImplFromFP(FILE *file_pointer) : fp(file_pointer) {}

    ~CSimpleStreamImplFromFP() override = default;

public:    // interface libCZI::IStream
    void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t *ptrBytesRead) override {
        fseeko(this->fp, offset, SEEK_SET);

        std::uint64_t bytesRead = fread(pv, 1, (size_t) size, this->fp);
        if (ptrBytesRead != nullptr) {
            *ptrBytesRead = bytesRead;
        }
    }
};

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

/// Read the metadata from the python file stream and return it as a string
/// \param self PyObject * to the object that called this code -- not used here but being consistent with convention
/// \param pyfp The python PyObject BytesIO object or relative there of the opened file
/// \return A string containing the XML metadata
static PyObject *cziread_meta_from_istream(PyObject *self, PyObject *pyfp) {
    auto stream = cziread_io_buffered_reader_to_istream(self, pyfp);
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

/// @brief remap libCZI::DimensionIndex from an integer Enum to a character Z(1) to 'Z', S(5) to 'S'
/// \param x the libCZI::DimensionIndex to map to the appropriate character
/// \return The character used to represent the DimensionIndex, one of {Z,C,T,R,S,I,H,V,B}
std::string map2String(const libCZI::DimensionIndex x){
    std::string ans;
    switch(x) {
        case libCZI::DimensionIndex::Z:
            ans = "Z";            ///< The Z-dimension.
            break;
        case libCZI::DimensionIndex::C:
            ans = "C";           ///< The C-dimension ("channel")
            break;
        case libCZI::DimensionIndex::T:
            ans = "T";
            break;
        case libCZI::DimensionIndex::R:
            ans = "R";
            break;
        case libCZI::DimensionIndex::S:
            ans = "S";
            break;
        case libCZI::DimensionIndex::I:
            ans = "I";
            break;
        case libCZI::DimensionIndex::H:
            ans = "H";
            break;
        case libCZI::DimensionIndex::V:
            ans = "V";
            break;
        case libCZI::DimensionIndex::B:
            ans = "B";
            break;
        default:
            throw std::out_of_range("Unsupported DimensionIndex!");
    }
    return ans;
}

/// @brief get_shape_from_fp returns the Dimensions of a ZISRAW/CZI when provided a ICZIReader object
/// \param czi: a shared_ptr to an initialized CziReader object
/// \return A Python Dictionary as a PyObject*
PyObject *get_shape_from_fp(std::shared_ptr <libCZI::ICZIReader> &czi){
    PyObject *ans = nullptr;
    auto statistics = czi->GetStatistics();
    std::map<libCZI::DimensionIndex, std::pair<int, int> > tbl;

    statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di, int start, int size)->bool{
        tbl.emplace( di, std::make_pair(start, size));
        return true;
    });

    PyObject* pyDict = PyDict_New();
    std::for_each(tbl.begin(), tbl.end(), [&pyDict](const std::pair< libCZI::DimensionIndex, std::pair<int, int> >& pare){
        std::string tmp = map2String(pare.first);
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
    try {
        auto stream = cziread_io_buffered_reader_to_istream(self, pyfp);
        auto cziReader = open_czireader_from_istream(stream);
        ans = get_shape_from_fp(cziReader);
    } catch (const exception &e){
        PyErr_SetString(PyExc_IOError, "Unable to lookup Dimensions in File.");
    }
    return ans;
}


static PyObject *cziread_allsubblocks_from_istream(PyObject *self, PyObject *args) {
    using namespace std::placeholders; // enable _1 _2 _3 type placeholders
    // parse arguments
    try {
        auto stream = cziread_io_buffered_reader_to_istream(self, args);
        auto cziReader = open_czireader_from_istream(stream);
        // count all the subblocks

        npy_intp subblock_count = 0;
        auto count_blocks([&subblock_count](int idx, const libCZI::SubBlockInfo &info)->bool{
            subblock_count++;
            return true;
        });

        // assignment warning is a CLION error it should be fine.
        std::function<bool(int, const libCZI::SubBlockInfo&)> countLambdaAsFunc =
                static_cast< std::function<bool(int, const libCZI::SubBlockInfo&)> >(count_blocks);

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

/// @brief Convert the python BytesIO / IOBufferedReader object to a child of libCZI::IStream
/// \param self PyObject * to the object that made the call -- not used here
/// \param pyfp PyObject * to the IOBufferedReader / BytesIO python stream of file contents
/// \return a shared_ptr to a CSimpleStreamImplFromFp -- a wrapper of a file pointer that is compatible with libCZI
std::shared_ptr <CSimpleStreamImplFromFP> cziread_io_buffered_reader_to_istream(PyObject *self, PyObject *pyfp) {
    // parse arguments
    PyObject *in_file;
    if (!PyArg_ParseTuple(pyfp, "O", &in_file)) throw BadArgsException();
    int fdescript = PyObject_AsFileDescriptor(in_file);
    if (fdescript == -1) throw BadFileDescriptorException();
    FILE *fp = fdopen(fdescript, "r");
    if (fp == nullptr) throw BadFileDescriptorException();
    return std::make_shared<CSimpleStreamImplFromFP>(fp);
}

/// @brief This function constructs and returns a CziReader class as a shared_ptr.
/// \param stream A CSimpleImplFromFP object say from \related cziread_io_buffered_reader_to_istream
/// \return a shared pointer to an initialized CziReader
std::shared_ptr <libCZI::ICZIReader> open_czireader_from_istream(std::shared_ptr <CSimpleStreamImplFromFP> stream) {
    // open the czi file
    auto cziReader = libCZI::CreateCZIReader();
    cziReader->Open(stream);
    return cziReader;
}


#endif //PYLIBCZI_AICS_ADDED_HPP
