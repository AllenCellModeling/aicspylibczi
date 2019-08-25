//
// Created by Jamie Sherman on 2019-08-18.
//
#include <tuple>
#include "aics_added.h"
#include "exceptions.h"

namespace pylibczi {

    void
    CSimpleStreamImplFromFP::Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t *ptrBytesRead) {
        fseeko(this->fp, offset, SEEK_SET);

        std::uint64_t bytesRead = fread(pv, 1, (size_t) size, this->fp);
        if (ptrBytesRead != nullptr)
            (*ptrBytesRead) = bytesRead;
    }

    Reader::Reader(FILE *f_in) : m_czireader(unique_ptr<CCZIReader>()) {
        if (!f_in) {
            throw FilePtrException("Reader class received a bad FILE *!");
        }
        auto istr = std::make_shared<CSimpleStreamImplFromFP>(f_in);
        m_czireader->Open(istr);
        m_statistics = m_czireader->GetStatistics();
    }

    std::string
    Reader::cziread_meta(){
        // get the the document's metadata
        auto mds = m_czireader->ReadMetadataSegment();
        auto md = mds->CreateMetaFromMetadataSegment();
        //auto docInfo = md->GetDocumentInfo();
        //auto dsplSettings = docInfo->GetDisplaySettings();
        std::string xml = md->GetXml();
        return xml;
    }

    bool Reader::isMosaicFile(void) {
        return (m_statistics.maxMindex > 0);
    }

    /// @brief get_shape_from_fp returns the Dimensions of a ZISRAW/CZI when provided a ICZIReader object
    /// \param czi: a shared_ptr to an initialized CziReader object
    /// \return A Python Dictionary as a PyObject*
    Reader::mapDiP
    Reader::get_shape() {
        mapDiP tbl;

        m_statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di, int start, int size) -> bool {
          tbl.emplace(di, std::make_pair(start, size));
          return true;
        });

        return tbl;
    }


    Reader::tuple_ans
    Reader::cziread_selected(libCZI::CDimCoordinate &planeCoord, int mIndex) {
        // count the matching subblocks
        ssize_t matching_subblock_count = 0;
        std::vector<IndexMap>  order_mapping;
        m_czireader->EnumerateSubBlocks([&](int idx, const libCZI::SubBlockInfo &info) -> bool {
          if (isPyramid0(info) && dimsMatch(planeCoord, info.coordinate)) {
              order_mapping.emplace_back(idx, info);
              matching_subblock_count++;
          }
          return true;
        });

        add_sort_order_index(order_mapping);

        // get scene index if specified
        int scene_index = -1;
        libCZI::IntRect sceneBox = {0, 0, -1, -1};
        if (planeCoord.TryGetPosition(libCZI::DimensionIndex::S, &scene_index)) {
            auto itt = m_statistics.sceneBoundingBoxes.find(scene_index);
            if (itt == m_statistics.sceneBoundingBoxes.end())
                sceneBox = itt->second.boundingBoxLayer0; // layer0 specific
            else
                sceneBox.Invalidate();
        } else {
            std::cout << "You are attempting to extract a scene from a single scene czi." << std::endl;
            scene_index = -1;
        }

        py::list images;
        std::vector<ssize_t> eshp{matching_subblock_count, 2};
        py::array_t<int32_t> coordinates(eshp);

        int32_t *coords = coordinates.mutable_data();
        // npy_int32 *coords = (npy_int32 *) PyArray_DATA(coordinates);

        ssize_t cnt = 0;
        m_czireader->EnumerateSubBlocks([&](int idx, const libCZI::SubBlockInfo &info) {

          if (!isPyramid0(info))
              return true;
          if (sceneBox.IsValid() && !sceneBox.IntersectsWith(info.logicalRect))
              return true;
          if (!dimsMatch(planeCoord, info.coordinate))
              return true;
          if (mIndex != -1 && info.mIndex != std::numeric_limits<int>::max() && mIndex != info.mIndex)
              return true;

          // add the sub-block image
          images.append(copy_bitmap_to_numpy_array(m_czireader->ReadSubBlock(idx)->CreateBitmap()));

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

        return std::make_tuple(images, coordinates, order_mapping);
        // return images;
    }


// private methods

    bool
    Reader::dimsMatch(const libCZI::CDimCoordinate &targetDims, const libCZI::CDimCoordinate &cziDims) {
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

    void
    Reader::add_sort_order_index(vector<IndexMap> &vec) {
        int counter = 0;
        std::sort(vec.begin(), vec.end(), [](IndexMap &a, IndexMap &b) -> bool { return (a < b); });
        for (auto &&a : vec)
            a.position(counter++);
        std::sort(vec.begin(), vec.end(),
                  [](IndexMap &a, IndexMap &b) -> bool { return a.lessThanSubblock(b); });
    }

    py::array
    Reader::copy_bitmap_to_numpy_array(std::shared_ptr<libCZI::IBitmapData> pBitmap) {
        // define numpy types/shapes and bytes per pixel depending on the zeiss bitmap pixel type.
        py::detail::npy_api::constants numpy_type = py::detail::npy_api::constants::NPY_UINT16_;
        std::string np_name;

        int pixel_size_bytes = 0;
        int channels = 1;
        auto size = pBitmap->GetSize();
        std::vector<py::ssize_t> shp{size.w, size.h};
        py::array img;
        // images in czi file are in F-order, set F-order flag (last argument to PyArray_Empty)
        switch (pBitmap->GetPixelType()) {
        case libCZI::PixelType::Gray8: // uint8
            pixel_size_bytes = 1;
            channels = 1;
            img = py::array_t<uint8_t, py::array::f_style>(shp);
            break;
        case libCZI::PixelType::Gray16: // uint16
            pixel_size_bytes = 2;
            channels = 1;
            img = py::array_t<uint16_t, py::array::f_style>(shp);
            break;
        case libCZI::PixelType::Bgr48: // uint16
            pixel_size_bytes = 6;
            channels = 3;
            shp.emplace(shp.begin(), channels); // {channels, size.w, size.h};
            img = py::array_t<uint16_t, py::array::f_style>(shp);
            break;
        default:throw PixelTypeException(pBitmap->GetPixelType(), "Unsupported type, ask libCZI to add support.");
        }

        // copy from the czi lib image pointer to the numpy array pointer
        void *pointer = img.mutable_data();
        std::size_t rowsize = size.w * pixel_size_bytes;
        {
            libCZI::ScopedBitmapLockerP lckScoped{pBitmap.get()};
            for (std::uint32_t h = 0; h < pBitmap->GetHeight(); ++h) {
                auto ptr = (((uint8_t *) lckScoped.ptrDataRoi) + h * lckScoped.stride);
                auto target = (((uint8_t *) pointer) + h * rowsize);
                std::memcpy(target, ptr, rowsize);
            }
        }
        return img;
    }

}
/*
/// @brief get_shape_from_fp returns the Dimensions of a ZISRAW/CZI when provided a ICZIReader object
/// \param czi: a shared_ptr to an initialized CziReader object
/// \return A Python Dictionary as a PyObject*
    std::map<char, int>
    get_shape_from_fp(std::shared_ptr<libCZI::ICZIReader> &czi) {
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




}

PYBIND11_MODULE(example, m) {
m.doc() = "pybind11 example plugin"; // optional module docstring

m.def("add", &pylibczi::add, "A function which adds two numbers");
}

 */
