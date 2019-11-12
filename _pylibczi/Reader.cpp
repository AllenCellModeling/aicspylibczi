#include <tuple>
#include <set>

#include "Reader.h"
#include "ImageFactory.h"
#include "exceptions.h"
#include "pylibczi_unistd.h"

namespace pylibczi {

  void
  CSimpleStreamImplFromFp::Read(std::uint64_t offset_, void* data_ptr_, std::uint64_t size_, std::uint64_t* bytes_read_ptr_)
  {
      fseeko(this->m_fp, offset_, SEEK_SET);

      std::uint64_t bytesRead = fread(data_ptr_, 1, (size_t) size_, this->m_fp);
      if (bytes_read_ptr_!=nullptr)
          (*bytes_read_ptr_) = bytesRead;
  }

  Reader::Reader(FileHolder f_in_)
      :m_czireader(new CCZIReader)
  {
      if (!f_in_.get()) {
          throw FilePtrException("Reader class received a bad FILE *!");
      }
      auto istr = std::make_shared<CSimpleStreamImplFromFp>(f_in_.get());
      m_czireader->Open(istr);
      m_statistics = m_czireader->GetStatistics();
  }

  std::string
  Reader::readMeta()
  {
      auto mds = m_czireader->ReadMetadataSegment();
      auto md = mds->CreateMetaFromMetadataSegment();
      std::string xml = md->GetXml();
      return xml;
  }

  bool Reader::isMosaic()
  {
      return (m_statistics.maxMindex>0);
  }

  /// @brief get_shape_from_fp returns the Dimensions of a ZISRAW/CZI when provided a ICZIReader object
  /// @param czi: a shared_ptr to an initialized CziReader object
  /// @return A Python Dictionary as a PyObject*
  Reader::MapDiP
  Reader::readDims()
  {
      MapDiP tbl;
      m_statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di_, int start_, int size_) -> bool {
          tbl.emplace(di_, std::make_pair(start_, size_));
          return true;
      });

      return tbl;
  }

  /// @brief get the Dimensions in the order they appear in
  /// @return a string containing the Dimensions for the image data object
  std::string
  Reader::dimsString()
  {
      std::string ans;
      m_statistics.dimBounds.EnumValidDimensions([&ans](libCZI::DimensionIndex di_, int start_, int size_) -> bool {
          ans += libCZI::Utils::DimensionToChar(di_);
          return true;
      });
      return ans;
  }

  std::pair<ImageVector, Reader::Shape>
  Reader::readSelected(libCZI::CDimCoordinate& plane_coord_, bool flatten_, int mIndex_)
  {
      ssize_t matchingSubblockCount = 0;
      std::vector<IndexMap> orderMapping;
      m_czireader->EnumerateSubBlocks([&](int index_, const libCZI::SubBlockInfo& info_) -> bool {
          if (isPyramid0(info_) && dimsMatch(plane_coord_, info_.coordinate)) {
              orderMapping.emplace_back(index_, info_);
              matchingSubblockCount++;
          }
          return true;
      });

      addSortOrderIndex(orderMapping);

      // get scene index if specified
      int sceneIndex;
      libCZI::IntRect sceneBox = {0, 0, -1, -1};
      if (plane_coord_.TryGetPosition(libCZI::DimensionIndex::S, &sceneIndex)) {
          auto itt = m_statistics.sceneBoundingBoxes.find(sceneIndex);
          if (itt==m_statistics.sceneBoundingBoxes.end())
              sceneBox = itt->second.boundingBoxLayer0; // layer0 specific
          else
              sceneBox.Invalidate();
      }
      else {
          sceneIndex = -1;
      }

      ImageVector images;
      images.reserve(matchingSubblockCount);

      m_czireader->EnumerateSubBlocks([&](int index_, const libCZI::SubBlockInfo& info_) {

          if (!isPyramid0(info_)) {
              return true;
          }
          if (sceneBox.IsValid() && !sceneBox.IntersectsWith(info_.logicalRect)) {
              return true;
          }
          if (!dimsMatch(plane_coord_, info_.coordinate)) {
              return true;
          }
          if (isMosaic() && mIndex_!=-1 && info_.mIndex!=std::numeric_limits<int>::min() && mIndex_!=info_.mIndex) {
              return true;
          }
          // add the sub-block image
          auto image = ImageFactory::constructImage(m_czireader->ReadSubBlock(index_)->CreateBitmap(),
              &info_.coordinate, info_.logicalRect, info_.mIndex);
          if (flatten_ && ImageFactory::numberOfChannels(image->pixelType())>1) {
              int start(0), sze(0);
              if (m_statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &start, &sze))
                  std::cerr << "Warning image has C: start(" << start << ") : size(" << sze << ") - how to handle channels?" << std::endl;
              auto splitImages = image->splitChannels(start+sze);
              for_each(splitImages.begin(), splitImages.end(), [&images](Image::ImVec::value_type& image_) { images.push_back(image_); });
          }
          else
              images.push_back(image);

          return true;
      });
      if (images.empty()) {
          throw pylibczi::CdimSelectionZeroImagesException(plane_coord_, m_statistics.dimBounds, "No pyramid0 selectable subblocks.");
      }
      auto shape = getShape(images, isMosaic());
      images.setMosaic(isMosaic());
      return std::make_pair(images, shape);
      // return images;
  }


// private methods

  bool
  Reader::dimsMatch(const libCZI::CDimCoordinate& target_dims_, const libCZI::CDimCoordinate& czi_dims_)
  {
      bool ans = true;
      target_dims_.EnumValidDimensions([&](libCZI::DimensionIndex dim, int value) -> bool {
          int cziDimValue = 0;
          if (czi_dims_.TryGetPosition(dim, &cziDimValue)) {
              ans = (cziDimValue==value);
          }
          return ans;
      });
      return ans;
  }

  void
  Reader::addSortOrderIndex(vector<IndexMap>& vector_of_index_maps_)
  {
      int counter = 0;
      std::sort(vector_of_index_maps_.begin(), vector_of_index_maps_.end(), [](IndexMap& a, IndexMap& b) -> bool { return (a<b); });
      for (auto&& a : vector_of_index_maps_)
          a.position(counter++);
      std::sort(vector_of_index_maps_.begin(), vector_of_index_maps_.end(),
          [](IndexMap& a_, IndexMap& b_) -> bool { return a_.lessThanSubBlock(b_); });
  }

  bool
  Reader::isValidRegion(const libCZI::IntRect& in_box_, const libCZI::IntRect& czi_box_)
  {
      bool ans = true;
      // check origin is in domain
      if (in_box_.x<czi_box_.x || czi_box_.x+czi_box_.w<in_box_.x) ans = false;
      if (in_box_.y<czi_box_.y || czi_box_.y+czi_box_.h<in_box_.y) ans = false;

      // check  (x1, y1) point is in domain
      int x1 = in_box_.x+in_box_.w;
      int y1 = in_box_.y+in_box_.h;
      if (x1<czi_box_.x || czi_box_.x+czi_box_.w<x1) ans = false;
      if (y1<czi_box_.y || czi_box_.y+czi_box_.h<y1) ans = false;

      if (!ans) throw RegionSelectionException(in_box_, czi_box_, "Requested region not in image!");
      if (in_box_.w<1 || 1>in_box_.h)
          throw RegionSelectionException(in_box_, czi_box_, "Requested region must have non-negative width and height!");

      return ans;
  }

  ImageVector
  Reader::readMosaic(libCZI::CDimCoordinate plane_coord_, float scale_factor_, libCZI::IntRect im_box_)
  {
      // handle the case where the function was called with region=None (default to all)
      if (im_box_.w==-1 && im_box_.h==-1) im_box_ = m_statistics.boundingBox;
      isValidRegion(im_box_, m_statistics.boundingBox); // if not throws RegionSelectionException

      std::map<libCZI::DimensionIndex, std::pair<int, int> > limitTbl;
      m_statistics.dimBounds.EnumValidDimensions([&limitTbl](libCZI::DimensionIndex di_, int start_, int size_) -> bool {
          limitTbl.emplace(di_, std::make_pair(start_, size_));
          return true;
      });

      auto accessor = m_czireader->CreateSingleChannelScalingTileAccessor();

      // multiTile accessor is not compatible with S, it composites the Scenes and the mIndexs together
      auto multiTileComposite = accessor->Get(
          im_box_,
          &plane_coord_,
          scale_factor_,
          nullptr);   // use default options

      // TODO how to handle 3 channel BGR image split them as in readSelected or ???
      auto image = ImageFactory::constructImage(multiTileComposite, &plane_coord_, im_box_, -1);
      ImageVector imageVector;
      imageVector.reserve(1);
      if (ImageFactory::numberOfChannels(image->pixelType())>1) {
          int start(0), sze(0);
          if (m_statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &start, &sze))
              std::cerr << "Warning image has C: start(" << start << ") : size(" << sze << ") - how to handle channels?" << std::endl;
          auto splitImages = image->splitChannels(start+sze);
          for_each(splitImages.begin(), splitImages.end(),
              [&imageVector](Image::ImVec::value_type& image_) { imageVector.push_back(image_); });
      }
      else
          imageVector.push_back(image);

      imageVector.setMosaic(isMosaic());
      return imageVector;
  }

  std::vector<std::pair<char, int> >
  Reader::getShape(pylibczi::ImageVector& images_, bool is_mosaic_)
  {
      using ImVec = pylibczi::Image::ImVec;
      std::sort(images_.begin(), images_.end(), [](ImVec::value_type& a_, ImVec::value_type& b_) {
          return *a_<*b_;
      });
      std::vector<std::vector<std::pair<char, int> > > validIndexes;
      for (const auto& image : images_) {
          validIndexes.push_back(image->getValidIndexes(is_mosaic_)); // only add M if it's a mosaic file
      }

      std::vector<std::pair<char, int> > charSizes;
      std::set<int> condensed;
      for (int i = 0; !validIndexes.empty() && i<validIndexes.front().size(); i++) {
          char c;
          for (const auto& vi : validIndexes) {
              c = vi[i].first;
              condensed.insert(vi[i].second);
          }
          charSizes.emplace_back(c, condensed.size());
          condensed.clear();
      }
      auto heightByWidth = images_.front()->shape(); // assumption: images are the same shape, if not 🙃
      charSizes.emplace_back('Y', heightByWidth[0]); // H
      charSizes.emplace_back('X', heightByWidth[1]); // W
      return charSizes;
  }

}
