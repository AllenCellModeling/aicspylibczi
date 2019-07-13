//
// Created by Jamie Sherman on 7/11/19.
//

#ifndef PYLIBCZI_AICS_ADDED_HPP
#define PYLIBCZI_AICS_ADDED_HPP

#include <cstdio>
#include <iostream>
#include <typeinfo>

#include "inc_libCZI.h"
#include "Python.h"

using namespace std;

class CSimpleStreamImplFromFP;

std::shared_ptr<CSimpleStreamImplFromFP> cziread_io_buffered_reader_to_istream(PyObject *self, PyObject *args);
std::shared_ptr<libCZI::ICZIReader> open_czireader_from_istream(std::shared_ptr<CSimpleStreamImplFromFP>stream);
PyArrayObject* copy_bitmap_to_numpy_array(std::shared_ptr<libCZI::IBitmapData> pBitmap);

/// <summary>	A wrapper that takes a FILE * and creates an IStream object out of it
class CSimpleStreamImplFromFP : public libCZI::IStream
{
private:
    FILE* fp;
public:
    CSimpleStreamImplFromFP() = delete;

    explicit CSimpleStreamImplFromFP(FILE* file_pointer): fp(file_pointer) {
        std::cout << "fp assigned." << std::endl;
        for(int i = 0; i < 8; i++){
            std::cout << ":" << std::endl;
            char c = getc(fp);
            std::cout << c << std::endl;
            std::cout << ":" << std::endl;
        }
    }
    ~CSimpleStreamImplFromFP() override = default;
public:	// interface libCZI::IStream
    void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override {
        int r = fseeko(this->fp, offset, SEEK_SET);

        std::uint64_t bytesRead = fread(pv, 1, (size_t)size, this->fp);
        if (ptrBytesRead != nullptr)
        {
            *ptrBytesRead = bytesRead;
        }
    }
};

static PyObject *cziread_meta_from_istream(PyObject *self, PyObject *args)
{
    auto stream = cziread_io_buffered_reader_to_istream(self, args);
    auto cziReader = open_czireader_from_istream(stream);

    // get the the document's metadata
    auto mds = cziReader->ReadMetadataSegment();
    auto md = mds->CreateMetaFromMetadataSegment();
    //auto docInfo = md->GetDocumentInfo();
    //auto dsplSettings = docInfo->GetDisplaySettings();
    std::string xml = md->GetXml();
    // copy the metadata into python string
    PyObject* pystring = Py_BuildValue("s", xml.c_str());

    cziReader->Close();
    return pystring;
}

static PyObject *cziread_allsubblocks_from_istream(PyObject *self, PyObject *args)
{
    // parse arguments
    auto stream = cziread_io_buffered_reader_to_istream(self, args);
    auto cziReader = open_czireader_from_istream(stream);

    // count all the subblocks
    npy_intp subblock_count = 0;
    cziReader->EnumerateSubBlocks(
            [&subblock_count](int idx, const libCZI::SubBlockInfo& info)
            {
                subblock_count++;
                return true;
            });
    //std::cout << "Enumerated " << subblock_count << std::endl;

    // meh - this seems to be not useful, what is an M-index? someone read the spec...
    //auto stats = cziReader->GetStatistics();
    //cout << stats.subBlockCount << " " << stats.maxMindex << endl;
    //int subblock_count = stats.subBlockCount;

    // copy the image data and coordinates into numpy arrays, return images as python list of numpy arrays
    PyObject* images = PyList_New(subblock_count);
    npy_intp eshp[2]; eshp[0] = subblock_count; eshp[1] = 2;
    PyArrayObject *coordinates = (PyArrayObject *) PyArray_Empty(2, eshp, PyArray_DescrFromType(NPY_INT32), 0);
    npy_int32 *coords = (npy_int32 *) PyArray_DATA(coordinates);

    npy_intp cnt = 0;
    cziReader->EnumerateSubBlocks(
            [&cziReader, &subblock_count, &cnt, images, coords](int idx, const libCZI::SubBlockInfo& info)
            {
                //std::cout << "Index " << idx << ": " << libCZI::Utils::DimCoordinateToString(&info.coordinate)
                //  << " Rect=" << info.logicalRect << " M-index " << info.mIndex << std::endl;

                // add the sub-block image
                PyList_SetItem(images, cnt,
                               (PyObject*) copy_bitmap_to_numpy_array(cziReader->ReadSubBlock(idx)->CreateBitmap()));
                // add the coordinates
                coords[2*cnt] = info.logicalRect.x; coords[2*cnt+1] = info.logicalRect.y;

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

std::shared_ptr<CSimpleStreamImplFromFP> cziread_io_buffered_reader_to_istream(PyObject *self, PyObject *args) {
    // parse arguments
    std::cout << "converting to istream!\n" << std::endl;
    PyObject *in_file;
    if (!PyArg_ParseTuple(args, "O", &in_file))
        return nullptr;
    auto fdescript = PyObject_AsFileDescriptor(in_file);
    auto fp = fdopen(fdescript,"r");
    if( fp == nullptr ){
        std::cerr << "Couldn't convert python object to file pointer." << std::endl;
        return nullptr;
    }

    std::cout << "conversion from python object to file pointer successful.(2)" << std::endl;
    std::cout << "fp is: " << fp << " with type: " << typeid(fp).name() << std::endl;
    return std::make_shared<CSimpleStreamImplFromFP>(fp);
}

std::shared_ptr<libCZI::ICZIReader> open_czireader_from_istream(std::shared_ptr<CSimpleStreamImplFromFP> stream) {
    // open the czi file
    auto cziReader = libCZI::CreateCZIReader();
    std::cout << "about to open stream!(2)" << std::endl;
    cziReader->Open(stream);
    std::cout << "cziReader opened stream" << std::endl;

    return cziReader;
}


#endif //PYLIBCZI_AICS_ADDED_HPP
