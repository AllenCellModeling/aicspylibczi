//
// Created by Jamie Sherman on 2019-09-19.
//

#include "pb_helpers.h"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <set>
#include <iostream>
#include "Image.h"
#include "Reader.h"
#include "exceptions.h"

namespace pb_helpers {

  using ImVec = pylibczi::ImageBC::ImVec;

  template<typename T>
  py::array* make_array(int new_size, std::vector<ssize_t>& shp, ImVec& imgs)
  {
	  if (new_size==0) return new py::array_t<T>({1}, new T);
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
	  auto char_sizes = pylibczi::Reader::get_shape(imgs, imgs.is_mosaic());
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
