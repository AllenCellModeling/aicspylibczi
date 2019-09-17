//
// Created by Jamie Sherman on 2019-09-11.
//

#ifndef _PYLIBCZI__PYLIBCZI_PB_HELPERS_H
#define _PYLIBCZI__PYLIBCZI_PB_HELPERS_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <set>
#include <iostream>
#include "Image.h"
#include "exceptions.h"

namespace py=pybind11;
namespace pb_helpers {

  using ImVec = pylibczi::ImageBC::ImVec;

  template<typename T>
  py::array* make_array(int new_size, std::vector<ssize_t>& shp, ImVec& imgs)
  {
	  T* ptr;
	  try {
		  ptr = new T[new_size];
	  }
	  catch (std::bad_alloc& ba) {
		  throw pylibczi::ImageCopyAllocFailed("try using a more constraints (S=1, T=5, etc on the DimensionIndex).", new_size);
	  }

	  pylibczi::ImageFactory image_factory;
	  T* pos = ptr;
	  for (auto img : imgs) {
		  auto typed_img = image_factory.get_derived<T>(img);
		  int len = typed_img->length();
		  std::copy(typed_img->get_raw_ptr(), typed_img->get_raw_ptr()+len, pos);
		  pos += len;
	  }

	  py::capsule free_when_done(ptr, [](void* f) {
		  T* ptr = reinterpret_cast<T*>(f);
		  delete[] ptr;
	  });

	  return new py::array_t<T>(shp, ptr, free_when_done);
  }

  py::array pack_array(pylibczi::ImageVector& imgs)
  {
	  // assumptions: The array contains images of the same size and the array is contiguous.
	  std::sort(imgs.begin(), imgs.end(), [](ImVec::value_type& a, ImVec::value_type& b) {
		  return *a<*b;
	  });
	  std::vector<std::vector<std::pair<char, int> > > valid_indexs;
	  for (const auto& img : imgs) {
		  valid_indexs.push_back(img->get_valid_indexs());
	  }

	  std::vector<std::pair<char, int> > char_sizes;
	  std::set<int> condensed;
	  for (int i = 0; i<valid_indexs.front().size(); i++) {
		  char c;
		  for (const auto& vi : valid_indexs) {
			  c = vi[i].first;
			  condensed.insert(vi[i].second);
		  }
		  char_sizes.emplace_back(c, condensed.size());
		  condensed.clear();
	  }
	  auto h_by_w = imgs.front()->shape(); // assumption: images are the same shape, if not 🙃
	  char_sizes.emplace_back('Y', h_by_w[0]); // H
	  char_sizes.emplace_back('X', h_by_w[1]); // W

	  int new_size = imgs.front()->length()*imgs.size();
	  std::vector<ssize_t> shp(char_sizes.size(), 0);
	  std::transform(char_sizes.begin(), char_sizes.end(), shp.begin(), [](const std::pair<char, int>& a) {
		  return a.second;
	  });
	  py::array* arr_p = nullptr;
	  switch (imgs.front()->pixelType()) {
	  case libCZI::PixelType::Gray8:
	  case libCZI::PixelType::Bgr24: arr_p = make_array<uint8_t>(new_size, shp, imgs);
		  break;
	  case libCZI::PixelType::Gray16:
	  case libCZI::PixelType::Bgr48: arr_p = make_array<uint16_t>(new_size, shp, imgs);
		  break;
	  case libCZI::PixelType::Gray32Float:
	  case libCZI::PixelType::Bgr96Float: arr_p = make_array<float>(new_size, shp, imgs);
		  break;
	  default: throw pylibczi::PixelTypeException(imgs.front()->pixelType(), "Unsupported pixel type");
	  }
	  return *arr_p;
  }

}

#endif //_PYLIBCZI__PYLIBCZI_PB_HELPERS_H
