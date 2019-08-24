//
// Created by Jamie Sherman on 2019-08-20.
//

#ifndef _PYLIBCZI__PYLIBCZI_EXCEPTIONS_H
#define _PYLIBCZI__PYLIBCZI_EXCEPTIONS_H

#include <exception>
#include <string>
#include "inc_libCZI.h"

namespace py = pybind11;

namespace pylibczi{

class FilePtrException : public std::exception {
        std::string m_msg;
    public:
      FilePtrException(const std::string &str): exception(), m_msg(str) {}
      const char * what () const throw () {
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
  PixelTypeException(libCZI::PixelType pt, const std::string &msg ): exception(), m_ptype(pt), m_msg(msg) {}

  const char * what () const throw () override {
      auto tname = m_byName.find(m_ptype);
      std:string name = (tname == m_byName.end()) ? "Unknown type" : tname->second;
      std::string tmp("PixelType( " + name + " ): " + m_msg);
      return tmp.c_str();
  }
};

/* FROM libCZI
 * enum class PixelType : std::uint8_t
	{
		Invalid = 0xff,				///< Invalid pixel type.
		Gray8 = 0,					///< Grayscale 8-bit unsinged.
		Gray16 = 1,					///< Grayscale 16-bit unsinged.
		Gray32Float = 2,			///< Grayscale 4 byte float.
		Bgr24 = 3,					///< BGR-color 8-bytes triples (memory order B, G, R).
		Bgr48 = 4,					///< BGR-color 16-bytes triples (memory order B, G, R).
		Bgr96Float = 8,				///< BGR-color 4 byte float triples (memory order B, G, R).
		Bgra32 = 9,					///< Currently not supported in libCZI.
		Gray64ComplexFloat = 10,	///< Currently not supported in libCZI.
		Bgr192ComplexFloat = 11,	///< Currently not supported in libCZI.
		Gray32 = 12,				///< Currently not supported in libCZI.
		Gray64Float = 13,			///< Currently not supported in libCZI.
	};*/
}


#endif //_PYLIBCZI__PYLIBCZI_EXCEPTIONS_H
