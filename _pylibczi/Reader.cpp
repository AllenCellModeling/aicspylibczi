//
// Created by Jamie Sherman on 2019-08-18.
//
#include <tuple>
#include "Reader.h"
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
    Reader::read_meta(){
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
    Reader::read_dims() {
        mapDiP tbl;

        m_statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di, int start, int size) -> bool {
          tbl.emplace(di, std::make_pair(start, size));
          return true;
        });

        return tbl;
    }


    Reader::tuple_ans
    Reader::read_selected(libCZI::CDimCoordinate &planeCoord, int mIndex) {
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

    template<typename T, size_t W, size_t H, size_t C>
    unique_ptr< std::array< std::array<std::array<T, W>, H>, C> >
    Reader::copy_bitmap_to_array(const std::shared_ptr<libCZI::IBitmapData>& pBitmap) {
        // this structure should be fordered but possibly with jumps between rows
        auto u_ptr = std::make_unique(std::array< std::array<std::array<T, W>, H>, C>{});
        // pixel_size = C * sizeof(T);
        size_t rowsize = W * C * sizeof(T);
        {
            size_t i = 0;
            libCZI::ScopedBitmapLockerP lckScoped{pBitmap.get()};
            for( auto c : *u_ptr.get() )
                for( auto h : c) {
                    auto src = static_cast<T *>(static_cast<uint8_t *>(lckScoped.ptrDataRoi)
                        + (&h - c.begin()) * lckScoped.stride);
                    std::copy(src, src + h.size(), h.begin());
                }
        }
        return u_ptr;
    }


    bool
    Reader::isValidRegion(const libCZI::IntRect &inBox, const libCZI::IntRect &cziBox ){
        bool ans = true;
        // check origin is in domain
        if( inBox.x < cziBox.x || cziBox.x + cziBox.w < inBox.x) ans = false;
        if( inBox.y < cziBox.y || cziBox.y + cziBox.h < inBox.y) ans = false;

        // check  (x1, y1) point is in domain
        int x1 = inBox.x + inBox.w;
        int y1 = inBox.y + inBox.h;
        if( x1 < cziBox.x || cziBox.x + cziBox.w < x1) ans = false;
        if( y1 < cziBox.y || cziBox.y + cziBox.h < y1) ans = false;

        if(!ans) throw RegionSelectionException( inBox, cziBox, "Requested region not in image!" );
        if(inBox.w < 1 || 1 > inBox.h)
            throw RegionSelectionException( inBox, cziBox, "Requested region must have non-negative width and height!" );

        return ans;
    }

    template<typename T, size_t W, size_t H, size_t C>
    unique_ptr< std::array< std::array<std::array<T, W>, H>, C> >
    Reader::read_mosaic(libCZI::IntRect imBox, const libCZI::CDimCoordinate &planeCoord, int scaleFactor) {
        // handle the case where the function was called with region=None (default to all)
        if ( imBox.w == -1 && imBox.h == -1 ) imBox = m_statistics.boundingBox;
        isValidRegion(imBox, m_statistics.boundingBox); // if not throws RegionSelectionException

        std::map< libCZI::DimensionIndex, std::pair< int, int> > limitTbl;
        m_statistics.dimBounds.EnumValidDimensions([&limitTbl](libCZI::DimensionIndex di, int start, int size)->bool{
          limitTbl.emplace( di, std::make_pair(start, size));
          return true;
        });

        auto accessor = m_czireader->CreateSingleChannelScalingTileAccessor();

        // multiTile accessor is not compatible with S, it composites the Scenes and the mIndexs together
        auto multiTileComposit = accessor->Get(
            imBox,
            &planeCoord,
            scaleFactor,
            nullptr);   // use default options

        auto img = copy_bitmap_to_array<T, W, H, C>(multiTileComposit);
        return img;
    }
}
