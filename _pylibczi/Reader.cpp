//
// Created by Jamie Sherman on 2019-08-18.
//
#include <tuple>
#include <unistd.h>
#include "Reader.h"
#include "exceptions.h"
#include <set>

namespace pylibczi {

  void
  CSimpleStreamImplFromFP::Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead)
  {
	  fseeko(this->fp, offset, SEEK_SET);

	  std::uint64_t bytesRead = fread(pv, 1, (size_t) size, this->fp);
	  if (ptrBytesRead!=nullptr)
		  (*ptrBytesRead) = bytesRead;
  }

  Reader::Reader(FileHolder f_in)
		  :m_czireader(new CCZIReader)
  {
	  if (!f_in.get()) {
		  throw FilePtrException("Reader class received a bad FILE *!");
	  }
	  auto istr = std::make_shared<CSimpleStreamImplFromFP>(f_in.get());
	  m_czireader->Open(istr);
	  m_statistics = m_czireader->GetStatistics();
  }

  std::string
  Reader::read_meta()
  {
	  // get the the document's metadata
	  auto mds = m_czireader->ReadMetadataSegment();
	  auto md = mds->CreateMetaFromMetadataSegment();
	  //auto docInfo = md->GetDocumentInfo();
	  //auto dsplSettings = docInfo->GetDisplaySettings();
	  std::string xml = md->GetXml();
	  return xml;
  }

  bool Reader::isMosaic()
  {
	  return (m_statistics.maxMindex>0);
  }

  /// @brief get_shape_from_fp returns the Dimensions of a ZISRAW/CZI when provided a ICZIReader object
  /// \param czi: a shared_ptr to an initialized CziReader object
  /// \return A Python Dictionary as a PyObject*
  Reader::mapDiP
  Reader::read_dims()
  {
	  mapDiP tbl;
//	  std::cout << "********* PID: " << getpid() << std::endl;
//	  sleep(10);
	  m_statistics.dimBounds.EnumValidDimensions([&tbl](libCZI::DimensionIndex di, int start, int size) -> bool {
		  tbl.emplace(di, std::make_pair(start, size));
		  return true;
	  });

	  return tbl;
  }

  std::pair<ImageVector, Reader::Shape>
  Reader::read_selected(libCZI::CDimCoordinate& planeCoord, bool flatten, int mIndex)
  {
  	int mcount = 1;
  	std::cout << "ReadSelected CDim: " << planeCoord << std::endl;
  	std::cout << "Bounds: " << m_statistics.dimBounds << std::endl;
	  std::cout << "RS: " << mcount++ << std::endl;
	  // count the matching subblocks
	  ssize_t matching_subblock_count = 0;
	  std::vector<IndexMap> order_mapping;
	  m_czireader->EnumerateSubBlocks([&](int idx, const libCZI::SubBlockInfo& info) -> bool {
		  if (isPyramid0(info) && dimsMatch(planeCoord, info.coordinate)) {
			  order_mapping.emplace_back(idx, info);
			  matching_subblock_count++;
		  }
		  return true;
	  });

	  std::cout << "matching_subblocks: " << matching_subblock_count << std::endl;
	  std::cout << "RS: " << mcount++ << std::endl;
	  add_sort_order_index(order_mapping);

	  std::cout << "RS: " << mcount++ << std::endl;
	  // get scene index if specified
	  int scene_index;
	  libCZI::IntRect sceneBox = {0, 0, -1, -1};
	  if (planeCoord.TryGetPosition(libCZI::DimensionIndex::S, &scene_index)) {
		  auto itt = m_statistics.sceneBoundingBoxes.find(scene_index);
		  if (itt==m_statistics.sceneBoundingBoxes.end())
			  sceneBox = itt->second.boundingBoxLayer0; // layer0 specific
		  else
			  sceneBox.Invalidate();
	  }
	  else {
		  scene_index = -1;
	  }

	  std::cout << "RS: " << mcount++ << std::endl;
	  ImageVector images;
	  images.reserve(matching_subblock_count);

	  std::cout << "RS: " << mcount++ << std::endl;
	  m_czireader->EnumerateSubBlocks([&](int idx, const libCZI::SubBlockInfo& info) {

		  if (!isPyramid0(info)) {
		  	  cout << "!pyramid0" << std::endl;
			  return true;
		  }
		  if (sceneBox.IsValid() && !sceneBox.IntersectsWith(info.logicalRect)) {
			  cout << "invalid box" << std::endl;
			  return true;
		  }
		  if (!dimsMatch(planeCoord, info.coordinate)) {
			  cout << "dims mismatch" << std::endl;
			  cout << "\t" << planeCoord << std::endl;
			  cout << "\t" << info.coordinate << std::endl;
			  return true;
		  }
		  if (isMosaic() && mIndex!=-1 && info.mIndex!=std::numeric_limits<int>::min() && mIndex!=info.mIndex) {
			  cout << "mIndex skip" << std::endl;
			  std::cout << "\t mIndex = " << mIndex << std::endl;
			  std::cout << "\t info.mIndex = " << info.mIndex << std::endl;
			  return true;
		  }
		  std::cout << "RS: " << mcount++ << std::endl;
		  // add the sub-block image
		  ImageFactory imageFactory;
		  auto img = imageFactory.construct_image(m_czireader->ReadSubBlock(idx)->CreateBitmap(),
				  &info.coordinate, info.logicalRect, info.mIndex);
		  if (flatten && ImageFactory::n_of_channels(img->pixelType())>1) {
			  int start(0), sze(0);
			  if (m_statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &start, &sze))
				  std::cerr << "Warning image has C: start(" << start << ") : size(" << sze << ") - how to handle channels?" << std::endl;
			  auto imgs = img->split_channels(start+sze);
			  for_each(imgs.begin(), imgs.end(), [&images](ImageBC::ImVec::value_type& iv) { images.push_back(iv); });
		  }
		  else
			  images.push_back(img);

		  return true;
	  });
	  std::cout << "read selected returning images" << std::endl;
	  if( images.empty() ){
	  	throw pylibczi::CdimSelectionZeroImagesExcetpion(planeCoord, m_statistics.dimBounds, "No pyramid0 selectable subblocks.");
	  }
	  auto shape = get_shape(images);
	  return std::make_pair(images, shape);
	  // return images;
  }


// private methods

  bool
  Reader::dimsMatch(const libCZI::CDimCoordinate& targetDims, const libCZI::CDimCoordinate& cziDims)
  {
	  bool ans = true;
	  targetDims.EnumValidDimensions([&](libCZI::DimensionIndex dim, int value) -> bool {
		  int cziDimValue = 0;
		  if (cziDims.TryGetPosition(dim, &cziDimValue)) {
			  ans = (cziDimValue==value);
		  }
		  return ans;
	  });
	  return ans;
  }

  void
  Reader::add_sort_order_index(vector<IndexMap>& vec)
  {
	  int counter = 0;
	  std::sort(vec.begin(), vec.end(), [](IndexMap& a, IndexMap& b) -> bool { return (a<b); });
	  for (auto&& a : vec)
		  a.position(counter++);
	  std::sort(vec.begin(), vec.end(),
			  [](IndexMap& a, IndexMap& b) -> bool { return a.lessThanSubblock(b); });
  }

  bool
  Reader::isValidRegion(const libCZI::IntRect& inBox, const libCZI::IntRect& cziBox)
  {
	  bool ans = true;
	  // check origin is in domain
	  if (inBox.x<cziBox.x || cziBox.x+cziBox.w<inBox.x) ans = false;
	  if (inBox.y<cziBox.y || cziBox.y+cziBox.h<inBox.y) ans = false;

	  // check  (x1, y1) point is in domain
	  int x1 = inBox.x+inBox.w;
	  int y1 = inBox.y+inBox.h;
	  if (x1<cziBox.x || cziBox.x+cziBox.w<x1) ans = false;
	  if (y1<cziBox.y || cziBox.y+cziBox.h<y1) ans = false;

	  if (!ans) throw RegionSelectionException(inBox, cziBox, "Requested region not in image!");
	  if (inBox.w<1 || 1>inBox.h)
		  throw RegionSelectionException(inBox, cziBox, "Requested region must have non-negative width and height!");

	  return ans;
  }

  ImageVector
  Reader::read_mosaic(const libCZI::CDimCoordinate& planeCoord, float scaleFactor, libCZI::IntRect imBox)
  {
	  // handle the case where the function was called with region=None (default to all)
	  if (imBox.w==-1 && imBox.h==-1) imBox = m_statistics.boundingBox;
	  isValidRegion(imBox, m_statistics.boundingBox); // if not throws RegionSelectionException

	  std::map<libCZI::DimensionIndex, std::pair<int, int> > limitTbl;
	  m_statistics.dimBounds.EnumValidDimensions([&limitTbl](libCZI::DimensionIndex di, int start, int size) -> bool {
		  limitTbl.emplace(di, std::make_pair(start, size));
		  return true;
	  });

	  auto accessor = m_czireader->CreateSingleChannelScalingTileAccessor();

	  // multiTile accessor is not compatible with S, it composites the Scenes and the mIndexs together
	  auto multiTileComposit = accessor->Get(
			  imBox,
			  &planeCoord,
			  scaleFactor,
			  nullptr);   // use default options

	  // TODO how to handle 3 channel BGR image split them as in read_selected or ???
	  ImageFactory imageFactory;
	  auto img = imageFactory.construct_image(multiTileComposit, &planeCoord, imBox, -1);
	  ImageVector image_vector;
	  if (ImageFactory::n_of_channels(img->pixelType())>1) {
		  int start(0), sze(0);
		  if (m_statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &start, &sze))
			  std::cerr << "Warning image has C: start(" << start << ") : size(" << sze << ") - how to handle channels?" << std::endl;
		  auto imgs = img->split_channels(start+sze);
		  for_each(imgs.begin(), imgs.end(), [&image_vector](ImageBC::ImVec::value_type& iv) { image_vector.push_back(iv); });
	  }
	  else
		  image_vector.push_back(img);

	  return image_vector;
  }

  std::vector<std::pair<char, int> >
  Reader::get_shape(pylibczi::ImageVector& imgs)
  {
	  using ImVec = pylibczi::ImageBC::ImVec;
	  std::cout << "Pack Array:A" << std::endl;
	  std::sort(imgs.begin(), imgs.end(), [](ImVec::value_type& a, ImVec::value_type& b) {
		  return *a<*b;
	  });
	  std::vector<std::vector<std::pair<char, int> > > valid_indexs;
	  for (const auto& img : imgs) {
		  valid_indexs.push_back(img->get_valid_indexs());
	  }

	  std::cout << "Pack Array:B1" << std::endl;
	  std::vector<std::pair<char, int> > char_sizes;
	  std::cout << "Pack Array:B2 " << valid_indexs.size() << std::endl;
	  std::set<int> condensed;
	  for (int i = 0; !valid_indexs.empty() && i<valid_indexs.front().size(); i++) {
		  std::cout << "Pack Array:B3" << std::endl;
		  char c;
		  for (const auto& vi : valid_indexs) {
			  std::cout << "Pack Array:B4" << std::endl;
			  std::cout << vi[i].first << " => " << vi[i].second << std::endl;
			  c = vi[i].first;
			  condensed.insert(vi[i].second);
		  }
		  char_sizes.emplace_back(c, condensed.size());
		  condensed.clear();
	  }
	  std::cout << "Pack Array:B3-2 " << imgs.size() << std::endl;
	  auto h_by_w = imgs.front()->shape(); // assumption: images are the same shape, if not 🙃
	  char_sizes.emplace_back('Y', h_by_w[0]); // H
	  char_sizes.emplace_back('X', h_by_w[1]); // W
	  return char_sizes;
  }

}
