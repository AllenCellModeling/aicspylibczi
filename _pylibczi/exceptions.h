#ifndef _PYLIBCZI_EXCEPTIONS_H
#define _PYLIBCZI_EXCEPTIONS_H

#include <exception>
#include <string>
#include <sstream>
#include <utility>
#include <iomanip>

#include "inc_libCZI.h"
#include "pylibczi_ostream.h"

namespace pylibczi {

  class FilePtrException: public std::exception {
      std::string m_message;
  public:
      explicit FilePtrException(std::string message_)
          :exception(), m_message(std::move(message_)) { }
      const char* what() const noexcept override
      {
          std::string tmp("File Pointer Exception: ");
          tmp += m_message;
          return tmp.c_str();
      }
  };

  class PixelTypeException: public std::exception {
      libCZI::PixelType m_pixelType;
      std::string m_message;

      static const std::map<libCZI::PixelType, const std::string> s_byName;

  public:
      PixelTypeException(libCZI::PixelType pixel_type_, std::string message_)
          :exception(), m_pixelType(pixel_type_), m_message(std::move(message_)) { }

      const char* what() const noexcept override
      {
          auto tname = s_byName.find(m_pixelType);
          std::string name((tname==s_byName.end()) ? "Unknown type" : tname->second);
          std::string tmp("PixelType( "+name+" ): "+m_message);
          return tmp.c_str();
      }

  };

  class RegionSelectionException: public std::exception {
      libCZI::IntRect m_requested;
      libCZI::IntRect m_image;
      std::string m_message;

  public:
      RegionSelectionException(const libCZI::IntRect& requested_box_, const libCZI::IntRect& image_box_, std::string message_)
          :m_requested(requested_box_), m_image(image_box_), m_message(std::move(message_)) { }

      const char* what() const noexcept override
      {
          std::stringstream front;
          front << "Requirement violated requested region is not a subset of the defined image! \n\t "
                << m_requested << " ⊄ " << m_image << "\n\t" << m_message << std::endl;
          return front.str().c_str();
      }

  };

  class ImageAccessUnderspecifiedException: public std::exception {
      int m_given, m_required;
      std::string m_message;
  public:
      ImageAccessUnderspecifiedException(int given_, int required_, std::string message_)
          :m_given(given_), m_required(required_), m_message(std::move(message_)) { }

      const char* what() const noexcept override
      {
          std::stringstream tmp;
          tmp << "Dimensions underspecified, given " << m_given << " dimensions but " << m_required << " needed! \n\t"
              << m_message << std::endl;
          return tmp.str().c_str();
      }
  };

  class ImageIteratorException: public std::exception {
      std::string m_message;
  public:
      explicit ImageIteratorException(const std::string &message_)
          :m_message("ImageIteratorException: "+message_) { }

      const char* what() const noexcept override
      {
          return m_message.c_str();
      }
  };

  class ImageSplitChannelException: public std::exception {
      std::string m_message;
      int m_channel;
  public:
      ImageSplitChannelException(const std::string &message_, int channel_)
          :m_message("ImageSplitChannelExcetion: "+message_), m_channel(channel_) { }

      const char* what() const noexcept override
      {
          std::stringstream tmp;
          tmp << m_message << " Channel should be zero or unset but has a value of " << m_channel
              << " not sure how to procede in assigning channels." << std::endl;
          return tmp.str().c_str();
      }

  };

  class ImageCopyAllocFailed: public std::bad_alloc {
      std::string m_message;
      unsigned long m_size;
  public:
      ImageCopyAllocFailed(std::string message_, unsigned long alloc_size_)
          :m_message(std::move(message_)), m_size(alloc_size_) { }

      const char* what() const noexcept override
      {
          std::stringstream tmp;
          auto gbSize = static_cast<double>(m_size);
          gbSize /= 1073741824.0; // 1024 * 1024 * 1024
          tmp << "ImageCopyAllocFailed [" << std::setprecision(1) << gbSize << " GB requested]: " << m_message << std::endl;
          return tmp.str().c_str();
      }
  };

  class CDimCoordinatesOverspecifiedException: public std::exception {
      std::string m_message;
  public:
      explicit CDimCoordinatesOverspecifiedException(std::string message_): m_message(std::move(message_)) {}

      const char* what() const noexcept override
      {
          std::stringstream tmp;
          tmp << "The coordinates are overspecified = you have specified a Dimension or Dimension value that is not valid. "
          << m_message << std::endl;
          return tmp.str().c_str();
      }
  };

  class CDimCoordinatesUnderspecifiedException: public std::exception {
      std::string m_message;
  public:
      explicit CDimCoordinatesUnderspecifiedException(std::string message_): m_message(std::move(message_)) {}

      const char* what() const noexcept override
      {
          std::stringstream tmp;
          tmp << "The coordinates are underspecified = you have not specified a Dimension that is required. "
              << m_message << std::endl;
          return tmp.str().c_str();
      }
  };

  class CdimSelectionZeroImagesException: public std::exception {
      libCZI::CDimCoordinate m_requestedPlaneCoordinate; // requested
      libCZI::CDimBounds m_planeCoordinateBounds; // image file
      std::string m_message;
  public:
      CdimSelectionZeroImagesException(libCZI::CDimCoordinate& requested_plane_coordinate_, libCZI::CDimBounds& plane_coordinate_bounds_,
          std::string message_)
          :m_requestedPlaneCoordinate(requested_plane_coordinate_), m_planeCoordinateBounds(plane_coordinate_bounds_),
           m_message(std::move(message_)) { std::cout << this->what(); }

      const char* what() const noexcept override
      {
          std::stringstream tmp;
          tmp << "Specified Dims resulted in NO image frames: " << m_requestedPlaneCoordinate << " ∉ " << m_planeCoordinateBounds
              << std::endl;
          return tmp.str().c_str();
      }
  };
}

#endif //_PYLIBCZI_EXCEPTIONS_H
