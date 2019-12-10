#include <tuple>
#include <set>
#include <utility>

#include "inc_libCZI.h"
#include "Reader.h"
#include "ImageFactory.h"
#include "exceptions.h"
#include "SubblockMetaVec.h"

namespace pylibczi {

  Reader::Reader(std::shared_ptr<libCZI::IStream> istream_)
      :m_czireader(new CCZIReader), m_specifyScene(false)
  {
      m_czireader->Open(std::move(istream_));
      m_statistics = m_czireader->GetStatistics();
      // create a reference for finding one or more subblock indices from a CDimCoordinate
      addOrderMapping(); // populate m_orderMapping
      checkSceneShapes();
  }

  Reader::Reader(const wchar_t* file_name_)
      :m_czireader(new CCZIReader), m_specifyScene(false)
  {
      std::shared_ptr<libCZI::IStream> sp;
      sp = std::shared_ptr<libCZI::IStream>(new CSimpleStreamImplCppStreams(file_name_));
      m_czireader->Open(sp);
      m_statistics = m_czireader->GetStatistics();
      // create a reference for finding one or more subblock indices from a CDimCoordinate
      addOrderMapping();// populate m_orderMapping
      checkSceneShapes();
  }

  void
  Reader::checkSceneShapes()
  {
      if (m_statistics.sceneBoundingBoxes.size()<2) return;
      libCZI::IntRect kept = m_statistics.sceneBoundingBoxes[0].boundingBoxLayer0;
      for_each(m_statistics.sceneBoundingBoxes.begin(),
          m_statistics.sceneBoundingBoxes.end(),
          [&](const auto& bbox_) {
              if (!m_specifyScene && (bbox_.second.boundingBoxLayer0.w!=kept.w || bbox_.second.boundingBoxLayer0.h!=kept.h)) {
                  std::cout << "WARNING: Scene must be specified for this file!" << std::endl;
                  m_specifyScene = true;
              }
          });

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
  Reader::DimensionRangeMap
  Reader::readDims()
  {
      DimensionIndexRangeMap tbl;
      m_statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di_, int start_, int size_) -> bool {
          tbl.emplace(di_, std::make_pair(start_, size_+start_-1)); // changed from [start, end) to be [start, end]
          return true;
      });

      DimensionRangeMap ans;
      for_each(tbl.begin(), tbl.end(), [&](const auto& pr_) {
          ans.emplace(dimToChar(pr_.first), pr_.second);
      });

      if( isMosaic() ) ans.emplace('M', std::make_pair(m_statistics.minMindex, m_statistics.maxMindex));

      libCZI::IntRect sbsize = getSceneYXSize();

      ans.emplace('Y', std::make_pair(0, sbsize.h-1));
      ans.emplace('X', std::make_pair(0, sbsize.w-1));
      return ans;
  }

  /// @brief get the size of each dimension
  /// @return vector of the integer sizes for each dimension
  std::vector<int>
  Reader::dimSizes()
  {
      if (m_specifyScene) {
          std::cout << "Shape of data is inconsistent between Scenes and must be read individually. "
                    << " File has " << m_statistics.sceneBoundingBoxes.size() << " Scenes." << std::endl;
          return std::vector<int>();
      }
      DimensionIndexRangeMap tbl;
      m_statistics.dimBounds.EnumValidDimensions([&](libCZI::DimensionIndex di_, int start_, int size_) -> bool {
          tbl.emplace(di_, std::make_pair(start_, size_)); // changed from [start, end) to be [start, end]
          return true;
      }); // sorts the letters into ascending order by default { Z, C, T, S }
      std::vector<int> ans(tbl.size());
      // use rbegin and rend to change it to ascending order.
      transform(tbl.rbegin(), tbl.rend(), ans.begin(), [](const auto& pr_) {
          return pr_.second.second;
      });

      if( isMosaic() ){
          ans.push_back(m_statistics.maxMindex + 1); // The M-index is zero based
      }

      libCZI::IntRect sbsize = getSceneYXSize();

      ans.push_back(sbsize.h);
      ans.push_back(sbsize.w);

      return ans;
  }

  libCZI::IntRect
  Reader::getSceneYXSize(int scene_index_)
  {
      if (m_statistics.dimBounds.IsValid(libCZI::DimensionIndex::S)) {
          int sStart(0), sSize(0);
          m_statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::S, &sStart, &sSize);
          if (scene_index_>=sStart && (sStart+sSize-1)<=scene_index_
              && !m_statistics.sceneBoundingBoxes.empty())
              return m_statistics.sceneBoundingBoxes[scene_index_].boundingBoxLayer0;
      }
      int index = m_orderMapping.front().second;
      auto subblk = m_czireader->ReadSubBlock(index);
      auto sbkInfo = subblk->GetSubBlockInfo();
      return sbkInfo.logicalRect;
  }

  /// @brief get the Dimensions in the order they appear in
  /// @return a string containing the Dimensions for the image data object
  std::string
  Reader::dimsString()
  {
      std::string ans;
      m_statistics.dimBounds.EnumValidDimensions([&ans](libCZI::DimensionIndex di_, int start_, int size_) -> bool {
          ans += Reader::dimToChar(di_);
          return true;
      });

      std::sort(ans.begin(), ans.end(), [](const char a_, const char b_) {
          return libCZI::Utils::CharToDimension(a_)>libCZI::Utils::CharToDimension(b_);
      });

      if( isMosaic() ) ans += "M";

      ans += "YX";
      return ans;
  }

  std::pair<ImageVector, Reader::Shape>
  Reader::readSelected(libCZI::CDimCoordinate& plane_coord_, int index_m_, bool split_bgr_)
  {
      int pos;
      if (m_specifyScene && !plane_coord_.TryGetPosition(libCZI::DimensionIndex::S, &pos)) {
          throw ImageAccessUnderspecifiedException(0, 1, "Scenes must be read individually "
                                                         "for this file, scenes have inconsistent YX shapes!""Scene");
      }
      SubblockSortable subblocksToFind(&plane_coord_, index_m_, isMosaic());
      SubblockIndexVec matches = getMatches(subblocksToFind);
      ImageVector images;
      images.reserve(matches.size());

      for_each(matches.begin(), matches.end(), [&](const SubblockIndexVec::value_type& match_) {
          auto subblock = m_czireader->ReadSubBlock(match_.second);
          const libCZI::SubBlockInfo& info = subblock->GetSubBlockInfo();
          auto image = ImageFactory::constructImage(subblock->CreateBitmap(),
              &info.coordinate, info.logicalRect, info.mIndex);
          if (split_bgr_ && ImageFactory::numberOfChannels(image->pixelType())>1) {
              int start(0), sze(0);
              if (m_statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &start, &sze))
                  std::cerr << "Warning image has C: start(" << start << ") : size(" << sze << ") - how to handle channels?" << std::endl;
              auto splitImages = image->splitChannels(start+sze);
              for_each(splitImages.begin(), splitImages.end(), [&images](Image::ImVec::value_type& image_) { images.push_back(image_); });
          }
          else
              images.push_back(image);
      });

      if (images.empty()) {
          throw pylibczi::CdimSelectionZeroImagesException(plane_coord_, m_statistics.dimBounds, "No pyramid0 selectable subblocks.");
      }
      images.setMosaic(isMosaic());
      auto shape = getShape(images, isMosaic());
      return std::make_pair(images, shape);
      // return images;
  }

  SubblockMetaVec
  Reader::readSubblockMeta(libCZI::CDimCoordinate& plane_coord_, int index_m_)
  {
      SubblockMetaVec metaSubblocks;
      metaSubblocks.setMosaic(isMosaic());

      SubblockSortable subBlockToFind(&plane_coord_, index_m_, isMosaic());
      SubblockIndexVec matches = getMatches(subBlockToFind);

      for_each(matches.begin(), matches.end(), [&](SubblockIndexVec::value_type& match_) {
          size_t metaSize = 0;
          auto subblock = m_czireader->ReadSubBlock(match_.second);
          auto sharedPtrString = subblock->GetRawData(libCZI::ISubBlock::Metadata, &metaSize);
          metaSubblocks.emplace_back(match_.first.coordinatePtr(), match_.first.mIndex(), isMosaic(), (char*) (sharedPtrString.get()));
      });

      return metaSubblocks;
  }

// private methods

  Reader::SubblockIndexVec
  Reader::getMatches(SubblockSortable& match_)
  {
      SubblockIndexVec ans;
      std::copy_if(m_orderMapping.begin(), m_orderMapping.end(), std::back_inserter(ans),
          [&match_](const SubblockIndexVec::value_type& a_) {
          return match_==a_.first;
      });
      if(ans.empty()){
          // check for invalid Dimension specification
          match_.coordinatePtr()->EnumValidDimensions([&](libCZI::DimensionIndex di_, int value_){
              bool keepGoing = true;
              if(!m_statistics.dimBounds.IsValid(di_)){
                  std::stringstream tmp;
                  tmp << dimToChar(di_) << " Not present in defined file Coordinates!";
                  throw CDimCoordinatesOverspecifiedException(tmp.str());
              }

              int start(0), size(0);
              m_statistics.dimBounds.TryGetInterval(di_, &start, &size);
              if( value_ < start || value_ >= (start + size)){
                  std::stringstream tmp;
                  tmp << dimToChar(di_) << " value " << value_ << "invalid, ∉ [" << start << ", "
                    << start + size << ")" << std::endl;
                  throw CDimCoordinatesOverspecifiedException(tmp.str());
              }
              return true;
          });
      }
      return ans;
  }

  void
  Reader::addOrderMapping()
  {
      // create a reference for finding one or more subblock indices from a CDimCoordinate
      m_czireader->EnumerateSubBlocks([&](int index_, const libCZI::SubBlockInfo& info_) -> bool {
          if (isPyramid0(info_)) {
              m_orderMapping.emplace_back(std::piecewise_construct,
                  std::make_tuple(&(info_.coordinate), info_.mIndex, isMosaic()),
                  std::make_tuple(index_)
              );
          }
          return true;
      });
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

      if( plane_coord_.IsValid(libCZI::DimensionIndex::S) ){
          throw CDimCoordinatesOverspecifiedException("Do not set S when reading mosaic files!");
      }

      if( !plane_coord_.IsValid(libCZI::DimensionIndex::C) ){
          throw CDimCoordinatesUnderspecifiedException( "C is not set, to read mosaic files you must specify C.");
      }
      SubblockSortable subBlockToFind(&plane_coord_, -1); // just check that the dims match something ignore that it's a mosaic file
      getMatches(subBlockToFind); // this does the checking

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

}
