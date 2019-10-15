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




} // namespace pylibczi

#endif //_PYLIBCZI__PYLIBCZI_IMAGE_H
