#ifndef _PYLIBCZI_IMAGE_H
#define _PYLIBCZI_IMAGE_H

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <libCZI/libCZI.h>
#include <libCZI/libCZI_Pixels.h>
#include <map>
#include <numeric>
#include <utility>
#include <vector>
#include <set>

#include "exceptions.h"
#include "helper_algorithms.h"
#include "SubblockSorter.h"

namespace pylibczi {

  /*!
   * @brief ImageBC is an abstract base class. The main reason for it's existence is to be able to polymorphically work with
   * Image<T> of different T through the virtual functions from a ImageBC *. Each ImageBC * is meant to point to the contents of
   * one subblock which may be either 2D or 3D, in the case of 3D the data is then later split into multiple 2D Image<T> so that
   * the concept of a Channel isn't destroyed.
   */
  class Image : public SubblockSorter {
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
      libCZI::IntRect m_xywh;  // (x0, y0, w, h) for image bounding box

      static std::map<libCZI::PixelType, std::string> s_pixelToTypeName;

  public:
      using ImVec = std::vector<std::shared_ptr<Image> >;

      Image(std::vector<size_t> shape_, libCZI::PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordinates_,
          libCZI::IntRect box_, int index_m_)
          : SubblockSorter(*plane_coordinates_, index_m_), m_shape(std::move(shape_)), m_pixelType(pixel_type_),
           m_xywh(box_) { }

      size_t calculateIdx(const std::vector<size_t>& indexes_);

      template<typename T>
      bool isTypeMatch();

      std::vector<size_t> shape() { return m_shape; }

      size_t length()
      {
          return std::accumulate(m_shape.begin(), m_shape.end(), (size_t) 1, std::multiplies<>());
      }

      libCZI::PixelType pixelType() { return m_pixelType; }

      virtual void loadImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_, size_t channels_) = 0;

      virtual ImVec splitChannels(int start_from_) = 0;
  };

  template<typename T>
  inline bool
  Image::isTypeMatch()
  {
      auto pt = s_pixelToTypeName[m_pixelType];
      return (typeid(T).name()==s_pixelToTypeName[m_pixelType]);
  }

  /*!
   * @brief This class is a std::vector< std::shared_ptr<ImageBC> >, it's sole reason for existing is to enable a custom binding
   * to convert the structure into a numpy.ndarray. From the C++ side it serves no purpose.
   */
  class ImageVector: public std::vector<std::shared_ptr<Image> > {
      bool m_isMosaic = false;

  public:
      void setMosaic(bool val_) { m_isMosaic = val_; }
      void sort(){
          std::sort(begin(), end(), [](const std::shared_ptr<Image> &a_, const std::shared_ptr<Image> &b_)->bool{
              return *a_ < *b_;
          });
      }

      std::vector<std::pair<char, int> >
      getShape(){
          std::vector<std::vector<std::pair<char, int> > > validIndexes;
          for (const auto& image : *this) {
              validIndexes.push_back(image->getValidIndexes(m_isMosaic)); // only add M if it's a mosaic file
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
          auto heightByWidth = front()->shape(); // assumption: images are the same shape, if not 🙃
          charSizes.emplace_back('Y', heightByWidth[0]); // H
          charSizes.emplace_back('X', heightByWidth[1]); // W
          return charSizes;
      }
  };

} // namespace pylibczi

#endif //_PYLIBCZI_IMAGE_H
