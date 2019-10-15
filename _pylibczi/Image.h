//
// Created by Jamie Sherman on 2019-08-28.
//

#ifndef _PYLIBCZI__PYLIBCZI_IMAGE_H
#define _PYLIBCZI__PYLIBCZI_IMAGE_H

#include "exceptions.h"
#include "Iterator.h"
#include "helper_algorithms.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <libCZI/libCZI.h>
#include <libCZI/libCZI_Pixels.h>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

namespace pylibczi {

// forward declare for use in casting in ImageBC
//  template<typename T>
//  class Image;

  //class ImageFactory;

  /*!
   * @brief ImageBC is an abstract base class. The main reason for it's existence is to be able to polymorphically work with
   * Image<T> of different T through the virtual functions from a ImageBC *. Each ImageBC * is meant to point to the contents of
   * one subblock which may be either 2D or 3D, in the case of 3D the data is then later split into multiple 2D Image<T> so that
   * the concept of a Channel isn't destroyed.
   */
  class ImageBC {
  protected:
      /*!
       * ImageBC holds the data describing the image (Image<T> inherits it). The m_matrixSizes can be 2D or 3D depending on
       * m_pixelType which holds the pixel type of the image (see libCZI). Examples: assuming a 1080p image
       * PixelType::Gray16 => 2D (1080, 1920)
       * PixelType::Bgr48 => 3D (3, 1080, 1920)
       * 3D images can be split up into 2D planes. This functionality is present for consistency with the Channel Dimension.
       */

      std::vector<size_t> m_shape; // C Y X order or Y X  ( H, W )  The shape of the data being stored
      libCZI::PixelType m_pixelType;
      libCZI::CDimCoordinate m_planeCoordinates;
      libCZI::IntRect m_xywh;  // (x0, y0, w, h) for image bounding box
      int m_mIndex;  // mIndex is a concept from libCZI and is used for mosaic files



      static std::map<libCZI::PixelType, std::string>
              s_pixelToTypeName;

  public:
      using ImVec = std::vector<std::shared_ptr<ImageBC> >;

      ImageBC(std::vector<size_t> shp, libCZI::PixelType pt, const libCZI::CDimCoordinate* cdim,
              libCZI::IntRect ir, int mIndex)
              :m_shape(std::move(shp)), m_pixelType(pt), m_planeCoordinates(*cdim),
               m_xywh(ir), m_mIndex(mIndex) { }

      size_t calculate_idx(const std::vector<size_t>& idxs);

      template<typename T>
      bool is_type_match();

      std::vector<size_t> shape() { return m_shape; }

      size_t length()
      {
          return std::accumulate(m_shape.begin(), m_shape.end(), (size_t) 1, std::multiplies<>());
      }

      std::vector<std::pair<char, int> > get_valid_indexs(bool isMosaic = false);

      bool operator<(ImageBC& other);

      libCZI::PixelType pixelType() { return m_pixelType; }

      virtual void load_image(const std::shared_ptr<libCZI::IBitmapData>& pBitmap, size_t channels) = 0;

      virtual ImVec split_channels(int startFrom) = 0;
  };

  template<typename T>
  inline bool
  ImageBC::is_type_match()
  {
      auto pt = s_pixelToTypeName[m_pixelType];
      return (typeid(T).name()==s_pixelToTypeName[m_pixelType]);
  }

  template<typename T>
  class Image: public ImageBC {
      std::unique_ptr<T[]> m_array;

  public:
      /*!
       * @brief The Image constructor creates the container and memory for storing an image from a ZeissRaw/CZI file. This class
       * is really intended to be created by ImageFactory.
       * @param shape The shape of the image can be in a vector or a tuple but <b>MUST be in {C, Y, X} order</b>.
       * @param pixel_type The Pixel type of the image,
       * @param cdim The coordinate structure used to define the plane, what scene, channel, time-point etc.
       * @param box The (x0, y0, w, h) structure containing the logical position of the image.
       * @param mIndex The mosaic index for the image, this is only relevant if the file is a mosaic file.
       */
      Image(std::vector<size_t> shape, libCZI::PixelType pixel_type, const libCZI::CDimCoordinate* cdim, libCZI::IntRect box, int mIndex)
              :ImageBC(shape, pixel_type, cdim, box, mIndex),
               m_array(new T[std::accumulate(shape.begin(), shape.end(), (size_t) 1, std::multiplies<>())])
      {
          if (!is_type_match<T>())
              throw PixelTypeException(m_pixelType, "Image asked to create a container for PixelType with inconsitent type.");
      }

      /*!
       * @brief the [] accessor, for accessing or changing a pixel value
       * @param idxsXY The X, Y coordinate in the plane (or X, Y, C} order if 3D. can be provided as an initializer list {x, y, c}
       * @return a reference to the pixel
       */
      T& operator[](const std::vector<size_t>& idxsXY);

      /*!
       * @brief an alternate accessor to the pixel value in CYX order
       * @param idxsCYX a vector or initializer list of C,Y,X indices
       * @return a reference to the pixel
       */
      T& getCYX(std::vector<size_t> idxsCYX) { return (*this)[std::vector<size_t>(idxsCYX.rbegin(), idxsCYX.rend())]; }

      /*!
       * @brief return the raw_pointer to the memory the image class contains, be careful with raw pointer manipulation. here be segfaults
       * @param jumpTo an integer offset from the beginning of the array.
       * @return a pointer to the internally managed memory. Image maintains ownership!
       */
      T* get_raw_ptr(int jumpTo = 0) { return m_array.get()+jumpTo; }

      /*!
       * return a pointer to the specified memory poisiton
       * @param lst a list of coordinates consistent with the internal storage
       * @return A pointer into the raw internal data (Image still maintains ownership of the memory).
       */
      T* get_raw_ptr(std::vector<size_t> lst); // inline definititon below

      /*!
       * @brief This function releases the memory from the container and gives it to the recipient to handle. The recipient takes
       * responsible for freeing the memory.
       * @return The raw pointer of type T*, where T is the storage type corresponding with the PixelType
       */
      T* release_memory()
      {
          if (!is_type_match<T>())
              throw PixelTypeException(pixelType(), "Image PixelType is inconsistent with requested memory type.");
          return m_array.release();
      }

      /*!
       * @brief Copy the image from the libCZI bitmap object into this Image object
       * @param pBitmap is the image bitmap from libCZI
       * @param channels the number of channels 1 for GrayX, 3 for BgrX etc. (ie the number of XY planes required to hold the image)
       */
      void load_image(const std::shared_ptr<libCZI::IBitmapData>& pBitmap,
              size_t channels) override;

      /*!
       * @brief If this container is a 3channel BGR image split it into single channel images so they can be merged into an data matrix
       * @param startFrom is an integer offset to start assigning the new channels from.
       * @return a vector of smart pointers wrapping Images (2D)
       */
      ImVec split_channels(int startFrom) override;
// TODO Implement set_sort_order() and operator()<
  };



  /*!
   * @brief This class is a std::vector< std::shared_ptr<ImageBC> >, it's sole reason for existing is to enable a custom binding
   * to convert the structure into a numpy.ndarray. From the C++ side it serves no purpose.
   */
  class ImageVector: public std::vector<std::shared_ptr<ImageBC> > {
      bool m_is_mosaic = false;

  public:
      bool is_mosaic() { return m_is_mosaic; }
      void set_mosaic(bool val) { m_is_mosaic = val; }

  };

  template<typename T>
  inline ImageBC::ImVec Image<T>::split_channels(int startFrom)
  {
      ImVec ivec;
      if (m_shape.size()<3)
          throw ImageSplitChannelException("Image  only has 2 dimensions. No channels to split.", 0);
      int cStart = 0;
      // TODO figure out if C can have a nonzero value for a BGR image
      if (m_planeCoordinates.TryGetPosition(libCZI::DimensionIndex::C, &cStart) && cStart!=0)
          throw ImageSplitChannelException("attempting to split channels", cStart);
      for (int i = 0; i<m_shape[0]; i++) {
          libCZI::CDimCoordinate tmp(m_planeCoordinates);
          tmp.Set(libCZI::DimensionIndex::C, i+startFrom); // assign the channel from the BGR
          // TODO should I change the pixel type from a BGRx to a Grayx/3
          ivec.emplace_back(new Image<T>({m_shape[1], m_shape[2]}, m_pixelType, &tmp, m_xywh, m_mIndex));
      }
      return ivec;
  }

  template<typename T>
  inline T& Image<T>::operator[](const std::vector<size_t>& idxs)
  {
      if (idxs.size()!=m_shape.size())
          throw ImageAccessUnderspecifiedException(idxs.size(), m_shape.size(), "from Image.operator[].");
      size_t idx = calculate_idx(idxs);
      return m_array[idx];
  }

  template<typename T>
  inline T* Image<T>::get_raw_ptr(std::vector<size_t> lst)
  {
      std::vector<size_t> zeroPadded(0, m_shape.size());
      std::copy(lst.rbegin(), lst.rend(), zeroPadded.rbegin());
      return this->operator[](calculate_idx(zeroPadded));
  }
  template<typename T>
  inline void Image<T>::load_image(const std::shared_ptr<libCZI::IBitmapData>& pBitmap, size_t channels)
  {
      libCZI::IntSize size = pBitmap->GetSize();
      {
          libCZI::ScopedBitmapLockerP lckScoped{pBitmap.get()};
          // WARNING do not compute the end of the array by multiplying stride by height, they are both uint32_t and you'll get an overflow for larger images
          uint8_t* sEnd = static_cast<uint8_t*>(lckScoped.ptrDataRoi)+lckScoped.size;
          SourceRange<T> sourceRange(channels, static_cast<T*>(lckScoped.ptrDataRoi), (T*) (sEnd), lckScoped.stride, size.w);
          TargetRange<T> targetRange(channels, size.w, size.h, m_array.get(), m_array.get()+length());
          for (std::uint32_t h = 0; h<pBitmap->GetHeight(); ++h) {
              paired_for_each(sourceRange.stride_begin(h), sourceRange.stride_end(h), targetRange.stride_begin(h),
                      [&](std::vector<T*> src, std::vector<T*> tgt) {
                          paired_for_each(src.begin(), src.end(), tgt.begin(), [&](T* s, T* t) {
                              *t = *s;
                          });
                      });
          }
      }
  }

} // namespace pylibczi

#endif //_PYLIBCZI__PYLIBCZI_IMAGE_H
