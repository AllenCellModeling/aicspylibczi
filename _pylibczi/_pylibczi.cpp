// This file is part of pylibczi.
// Copyright (c) 2018 Center of Advanced European Studies and Research (caesar)
//
// pylibczi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pylibczi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pylibczi.  If not, see <https://www.gnu.org/licenses/>.

// cpp wrapper for libCZI for reading Zeiss czi file scene data.

#include "Python.h"
#define NPY_NO_DEPRECATED_API NPY_1_14_API_VERSION
#include "numpy/arrayobject.h"

#include <iostream>
#include <vector>

#include "inc_libCZI.h"

// https://stackoverflow.com/questions/3342726/c-print-out-enum-value-as-text
std::ostream& operator<<(std::ostream& out, const libCZI::PixelType value){
    static std::map<libCZI::PixelType, std::string> strings;
    if (strings.size() == 0){
#define INSERT_ELEMENT(p) strings[p] = #p
        INSERT_ELEMENT(libCZI::PixelType::Invalid);
        INSERT_ELEMENT(libCZI::PixelType::Gray8);
        INSERT_ELEMENT(libCZI::PixelType::Gray16);
        INSERT_ELEMENT(libCZI::PixelType::Gray32Float);
        INSERT_ELEMENT(libCZI::PixelType::Bgr24);
        INSERT_ELEMENT(libCZI::PixelType::Bgr48);
        INSERT_ELEMENT(libCZI::PixelType::Bgr96Float);
        INSERT_ELEMENT(libCZI::PixelType::Bgra32);
        INSERT_ELEMENT(libCZI::PixelType::Gray64ComplexFloat);
        INSERT_ELEMENT(libCZI::PixelType::Bgr192ComplexFloat);
        INSERT_ELEMENT(libCZI::PixelType::Gray32);
        INSERT_ELEMENT(libCZI::PixelType::Gray64Float);
#undef INSERT_ELEMENT
    }

    return out << strings[value];
}

std::ostream& operator<<(std::ostream& out, const libCZI::DimensionIndex value){
    static std::map<libCZI::DimensionIndex, std::string> strings;
    if (strings.size() == 0){
#define INSERT_ELEMENT(p) strings[p] = #p
        INSERT_ELEMENT(libCZI::DimensionIndex::invalid);
        INSERT_ELEMENT(libCZI::DimensionIndex::MinDim);
        INSERT_ELEMENT(libCZI::DimensionIndex::Z);
        INSERT_ELEMENT(libCZI::DimensionIndex::C);
        INSERT_ELEMENT(libCZI::DimensionIndex::T);
        INSERT_ELEMENT(libCZI::DimensionIndex::R);
        INSERT_ELEMENT(libCZI::DimensionIndex::S);
        INSERT_ELEMENT(libCZI::DimensionIndex::I);
        INSERT_ELEMENT(libCZI::DimensionIndex::H);
        INSERT_ELEMENT(libCZI::DimensionIndex::V);
        INSERT_ELEMENT(libCZI::DimensionIndex::B);
        INSERT_ELEMENT(libCZI::DimensionIndex::MaxDim);
#undef INSERT_ELEMENT
    }

    return out << strings[value];
}

/* #### Globals #################################### */

// .... Python callable extensions ..................

static PyObject *cziread_meta(PyObject *self, PyObject *args);
static PyObject *cziread_scene(PyObject *self, PyObject *args);
static PyObject *cziread_allsubblocks(PyObject *self, PyObject *args);

/* ==== Set up the methods table ====================== */
static PyMethodDef _pylibcziMethods[] = {
    {"cziread_meta", cziread_meta, METH_VARARGS, "Read czi meta data"},
    {"cziread_scene", cziread_scene, METH_VARARGS, "Read czi scene image"},
    {"cziread_allsubblocks", cziread_allsubblocks, METH_VARARGS, "Read czi image containing all scenes"},

    {NULL, NULL, 0, NULL}        /* Sentinel */
};

// https://docs.python.org/3.6/extending/extending.html
// http://python3porting.com/cextensions.html

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "_pylibczi",           /* m_name */
        NULL,                /* m_doc */
        -1,                  /* m_size */
        _pylibcziMethods,      /* m_methods */
        NULL,                /* m_reload */
        NULL,                /* m_traverse */
        NULL,                /* m_clear */
        NULL                 /* m_free */
};

// generic exception for any errors encountered here
static PyObject *PylibcziError;

extern "C" {
PyMODINIT_FUNC PyInit__pylibczi(void)
{
    PyObject *module = PyModule_Create(&moduledef);

    if (module == NULL)
        return NULL;

    PylibcziError = PyErr_NewException("pylibczi.error", NULL, NULL);
    Py_INCREF(PylibcziError);
    PyModule_AddObject(module, "_pylibczi_exception", PylibcziError);

    import_array();  // Must be present for NumPy.  Called first after above line.

    return module;
}
}

/* #### Helper prototypes ################################### */

std::shared_ptr<libCZI::ICZIReader> open_czireader_from_cfilename(char const *fn);
PyArrayObject* copy_bitmap_to_numpy_array(std::shared_ptr<libCZI::IBitmapData> pBitmap);

/* #### Extended modules #################################### */

static PyObject *cziread_meta(PyObject *self, PyObject *args) {
    char *filename_buf;
    // parse arguments
    if (!PyArg_ParseTuple(args, "s", &filename_buf))
        return NULL;

    auto cziReader = open_czireader_from_cfilename(filename_buf);

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

static PyObject *cziread_allsubblocks(PyObject *self, PyObject *args) {
    char *filename_buf;
    // parse arguments
    if (!PyArg_ParseTuple(args, "s", &filename_buf))
        return NULL;

    auto cziReader = open_czireader_from_cfilename(filename_buf);

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

static PyObject *cziread_scene(PyObject *self, PyObject *args) {
    char *filename_buf;
    PyArrayObject *scene_or_box;

    // parse arguments
    if (!PyArg_ParseTuple(args, "sO!", &filename_buf, &PyArray_Type, &scene_or_box))
        return NULL;

    // get either the scene or a bounding box on the scene to load
    npy_intp size_scene_or_box = PyArray_SIZE(scene_or_box);
    if( PyArray_TYPE(scene_or_box) != NPY_INT64 ) {
        PyErr_SetString(PylibcziError, "Scene or box argument must be int64");
        return NULL;
    }
    npy_int64 *ptr_scene_or_box = (npy_int64*) PyArray_DATA(scene_or_box);
    bool use_scene; npy_int32 scene = -1; npy_int32 rect[4];
    if( size_scene_or_box == 1 ) {
        use_scene = true;
        scene = ptr_scene_or_box[0];
    } else if( size_scene_or_box == 4 ) {
        use_scene = false;
        for( int i=0; i < 4; i++ ) rect[i] = ptr_scene_or_box[i];
    } else {
        PyErr_SetString(PylibcziError, "Second input must be size 1 (scene) or 4 (box)");
        return NULL;
    }

    auto cziReader = open_czireader_from_cfilename(filename_buf);

    // if only the scene was given the enumerate subblocks to get limits, otherwise use the provided bounding box.
    int min_x, min_y, max_x, max_y, size_x, size_y;
    //std::vector<bool> valid_dims ((int) libCZI::DimensionIndex::MaxDim, false);
    if( use_scene ) {
        // enumerate subblocks, get the min and max coordinates of the specified scene
        min_x = std::numeric_limits<int>::max(); min_y = std::numeric_limits<int>::max(); max_x = -1; max_y = -1;
        cziReader->EnumerateSubBlocks(
            //[scene, &min_x, &min_y, &max_x, &max_y, &valid_dims](int idx, const libCZI::SubBlockInfo& info)
            [scene, &min_x, &min_y, &max_x, &max_y](int idx, const libCZI::SubBlockInfo& info)
        {
            int cscene = 0;
            info.coordinate.TryGetPosition(libCZI::DimensionIndex::S, &cscene);
            // negative value for scene indicates to load all scenes
            if( cscene == scene || scene < 0 ) {
                //cout << "Index " << idx << ": " << libCZI::Utils::DimCoordinateToString(&info.coordinate)
                //  << " Rect=" << info.logicalRect << " scene " << scene << endl;
                auto rect = info.logicalRect;
                if( rect.x < min_x ) min_x = rect.x;
                if( rect.y < min_y ) min_y = rect.y;
                if( rect.x + rect.w > max_x ) max_x = rect.x + rect.w;
                if( rect.y + rect.h > max_y ) max_y = rect.y + rect.h;
            }

            //info.coordinate.EnumValidDimensions(
            //    [&valid_dims](libCZI::DimensionIndex dim, int value)
            //{
            //    valid_dims[(int) dim] = true;
            //    //cout << "Dimension  " << dim << " value " << value << endl;
            //    return true;
            //});

            return true;
        });
        size_x = max_x-min_x; size_y = max_y-min_y;
    } else {
        min_x = rect[0]; size_x = rect[2]; min_y = rect[1]; size_y = rect[3];
        max_x = min_x + size_x; max_y = min_y + size_y;
    }
    //std::cout << "min x y " << min_x << " " << min_y << " max x y " << max_x << " " << max_y << std::endl;
    //for (auto it = valid_dims.begin(); it != valid_dims.end(); ++it) {
    //    if( *it ) {
    //        int index = std::distance(valid_dims.begin(), it);
    //        std::cout << static_cast<libCZI::DimensionIndex>(index) << ' ';
    //    }
    //}
    //cout << endl;

    // get the accessor to the image data
    auto accessor = cziReader->CreateSingleChannelTileAccessor();
    // xxx - how to generalize correct image dimension here?
    //   commented code above creates bool vector saying which dims are valid (in any subblock).
    //   it is possible for a czi file to not have any valid dims, not sure what this means exactly.
    //libCZI::CDimCoordinate planeCoord{ { libCZI::DimensionIndex::Z,0 } };
    libCZI::CDimCoordinate planeCoord{ { libCZI::DimensionIndex::C,0 } };
    auto multiTileComposit = accessor->Get(
        libCZI::IntRect{ min_x, min_y, size_x, size_y },
        &planeCoord,
        nullptr);   // use default options

    PyArrayObject* img = copy_bitmap_to_numpy_array(multiTileComposit);

    cziReader->Close();
    return (PyObject*) img;
}

PyArrayObject* copy_bitmap_to_numpy_array(std::shared_ptr<libCZI::IBitmapData> pBitmap) {
    // define numpy types/shapes and bytes per pixel depending on the zeiss bitmap pixel type.
    int numpy_type = NPY_UINT16; int pixel_size_bytes = 0; int channels = 1;
    switch( pBitmap->GetPixelType() ) {
        case libCZI::PixelType::Gray8:
            numpy_type = NPY_UINT8; pixel_size_bytes = 1; channels = 1;
            break;
        case libCZI::PixelType::Gray16:
            numpy_type = NPY_UINT16; pixel_size_bytes = 2; channels = 1;
            break;
        case libCZI::PixelType::Bgr48:
            numpy_type = NPY_UINT16; pixel_size_bytes = 6; channels = 3;
            break;
        default:
            std::cout << pBitmap->GetPixelType() << std::endl;
            PyErr_SetString(PylibcziError, "Unknown image type in czi file, ask to add more types.");
            return NULL;
    }

    // allocate the numpy matrix to copy image into
    //cout << size_x << " " << size_y << endl;
    auto size = pBitmap->GetSize();
    int size_x = size.w, size_y = size.h;
    PyArrayObject *img;
    int swap_axes[2];
    if( channels==1 ) {
        npy_intp shp[2]; shp[0] = size_x; shp[1] = size_y;
        // images in czi file are in F-order, set F-order flag (last argument to PyArray_Empty)
        img = (PyArrayObject *) PyArray_Empty(2, shp, PyArray_DescrFromType(numpy_type), 1);
        swap_axes[0] = 0; swap_axes[1] = 1;
    } else {
        npy_intp shp[3]; shp[1] = size_x; shp[2] = size_y; shp[0] = channels;
        // images in czi file are in F-order, set F-order flag (last argument to PyArray_Empty)
        img = (PyArrayObject *) PyArray_Empty(3, shp, PyArray_DescrFromType(numpy_type), 1);
        swap_axes[0] = 0; swap_axes[1] = 2;
    }
    void *pointer = PyArray_DATA(img);

    // copy from the czi lib image pointer to the numpy array pointer
    auto bitmap = pBitmap->Lock();
    //cout << "sixe_x " << size_x << " size y " << size_y << endl;
    //cout << "stride " << bitmap.stride << " size " << bitmap.size << endl;
    // can not do a single memcpy call because the stride does not necessarily match the row size.
    npy_byte *cptr = (npy_byte*)pointer, *cimgptr = (npy_byte*)bitmap.ptrDataRoi;
    // stride units is not documented but emperically means the row (x) stride in bytes, not in pixels.
    int rowsize = pixel_size_bytes * size_x; //, imgrowsize = pixel_size_bytes * bitmap.stride;
    for( int y=0; y < size_y; y++ ) {
        std::memcpy(cptr, cimgptr, rowsize);
        cptr += rowsize; cimgptr += bitmap.stride;
    }
    pBitmap->Unlock();

    // transpose to convert from F-order to C-order array
    return (PyArrayObject*) PyArray_SwapAxes(img,swap_axes[0],swap_axes[1]);
}

std::shared_ptr<libCZI::ICZIReader> open_czireader_from_cfilename(char const *fn) {
    // open the czi file
    // https://msdn.microsoft.com/en-us/library/ms235631.aspx
    size_t newsize = strlen(fn) + 1;
    // The following creates a buffer large enough to contain
    // the exact number of characters in the original string
    // in the new format. If you want to add more characters
    // to the end of the string, increase the value of newsize
    // to increase the size of the buffer.
    wchar_t * wcstring = new wchar_t[newsize];
    // Convert char* string to a wchar_t* string.
    //size_t convertedChars = mbstowcs(wcstring, fn, newsize);
    mbstowcs(wcstring, fn, newsize);
    auto cziReader = libCZI::CreateCZIReader();
    auto stream = libCZI::CreateStreamFromFile(wcstring);
    delete[] wcstring;
    cziReader->Open(stream);

    return cziReader;
}
