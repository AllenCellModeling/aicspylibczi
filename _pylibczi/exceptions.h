//
// Created by Jamie Sherman on 2019-08-20.
//

#ifndef _PYLIBCZI__PYLIBCZI_EXCEPTIONS_H
#define _PYLIBCZI__PYLIBCZI_EXCEPTIONS_H

#include <exception>
#include <string>
#include <sstream>
#include <utility>
#include "inc_libCZI.h"

namespace pylibczi {

  class FilePtrException: public std::exception {
	  std::string m_msg;
  public:
	  explicit FilePtrException(std::string str)
			  :exception(), m_msg(std::move(str)) { }
	  const char* what() const noexcept override
	  {
		  std::string tmp("File Pointer Exception: ");
		  tmp += m_msg;
		  return tmp.c_str();
	  }
  };

  class PixelTypeException: public std::exception {
	  libCZI::PixelType m_ptype;
	  std::string m_msg;

	  std::map<libCZI::PixelType, const std::string> m_byName = {
			  {libCZI::PixelType::Invalid, "Invalid"},
			  {libCZI::PixelType::Gray8, "Gray8"},
			  {libCZI::PixelType::Gray16, "Gray16"},
			  {libCZI::PixelType::Gray32Float, "Gray32Float"},
			  {libCZI::PixelType::Bgr24, "Bgr24"},
			  {libCZI::PixelType::Bgr48, "Bgr48"},
			  {libCZI::PixelType::Bgr96Float, "Bgr96Float"},
			  {libCZI::PixelType::Bgra32, "Bgra32"},
			  {libCZI::PixelType::Gray64ComplexFloat, "Gray64ComplexFloat"},
			  {libCZI::PixelType::Bgr192ComplexFloat, "Bgr192ComplexFloat"},
			  {libCZI::PixelType::Gray32, "Gray32"},
			  {libCZI::PixelType::Gray64Float, "Gray64Float"}
	  };

  public:
	  PixelTypeException(libCZI::PixelType pt, std::string msg)
			  :exception(), m_ptype(pt), m_msg(std::move(msg)) { }

	  const char* what() const noexcept override
	  {
		  auto tname = m_byName.find(m_ptype);
		  std::string name((tname==m_byName.end()) ? "Unknown type" : tname->second);
		  std::string tmp("PixelType( "+name+" ): "+m_msg);
		  return tmp.c_str();
	  }

  };

  class RegionSelectionException: public std::exception {
	  libCZI::IntRect m_requested;
	  libCZI::IntRect m_image;
	  std::string m_msg;

  public:
	  RegionSelectionException(const libCZI::IntRect& req, const libCZI::IntRect& im, std::string msg)
			  :m_requested(req), m_image(im), m_msg(std::move(msg)) { }

	  const char* what() const noexcept override
	  {
		  std::stringstream front;
		  front << "Requirement violated requested region is not a subset of the defined image! \n\t "
		        << m_requested << " ⊄ " << m_image << "\n\t" << m_msg << std::endl;
		  return front.str().c_str();
	  }

  };

  class ImageAccessUnderspecifiedException: public std::exception {
	  int m_given, m_required;
	  std::string m_msg;
  public:
	  ImageAccessUnderspecifiedException(int given, int &required, std::string msg)
			  :m_given(given), m_required(required), m_msg(std::move(msg)) { }

	  const char* what() const noexcept override
	  {
		  std::stringstream tmp;
		  tmp << "Dimensions underspecified, given " << m_given << " dimensions but " << m_required << " needed! \n\t"
		      << m_msg << std::endl;
		  return tmp.str().c_str();
	  }
  };

  class ImageIteratorException: public std::exception {
	  std::string m_msg;
  public:
	  explicit ImageIteratorException(std::string msg)
			  :m_msg("ImageIteratorException: "+msg)
	  {
		  int x = 5;
	  }
	  const char* what() const noexcept override
	  {
		  return m_msg.c_str();
	  }
  };

  class ImageSplitChannelException: public std::exception {
	  std::string m_msg;
	  int m_channel;
  public:
	  ImageSplitChannelException(std::string msg, int channel)
			  :m_msg("ImageSplitChannelExcetion: "+msg), m_channel(channel)
	  {}

	  const char* what() const noexcept override {
		  std::stringstream tmp;
		  tmp << m_msg << " Channel should be zero or unset but has a value of " << m_channel
		      << " not sure how to procede in assigning channels." << std::endl;
		  return tmp.str().c_str();
	  }

  };
}

#endif //_PYLIBCZI__PYLIBCZI_EXCEPTIONS_H
