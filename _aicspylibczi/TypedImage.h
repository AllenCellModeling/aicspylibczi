#ifndef _PYLIBCZI_TYPEDIMAGE_H
#define _PYLIBCZI_TYPEDIMAGE_H

#include <functional>

#include "Image.h"
#include "SourceRange.h"
#include "TargetRange.h"
#include "ImageFactory.h"

namespace pylibczi {

  template<typename T>
  class TypedImage: public Image {
      T* m_array; // raw pointer to memory in ImagesContainer

  public:
      /*!
       * @brief The Image constructor creates the container and memory for storing an image from a ZeissRaw/CZI file. This class
       * is really intended to be created by ImageFactory.
       * @param shape_ The shape of the image can be in a vector or a tuple but <b>MUST be in {C, Y, X} order</b>.
       * @param pixel_type_ The Pixel type of the image,
       * @param plane_coordantes_ The coordinate structure used to define the plane, what scene, channel, time-point etc.
       * @param box_ The (x0, y0, w, h) structure containing the logical position of the image.
       * @param mem_ptr_ The raw pointer to the position to write the image into.
       * @param m_index_ The mosaic index for the image, this is only relevant if the file is a mosaic file.
       */
      TypedImage(std::vector<size_t> shape_, libCZI::PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordantes_,
          libCZI::IntRect box_, T* mem_ptr_, int m_index_)
          :Image(shape_, pixel_type_, plane_coordantes_, box_, m_index_),
           m_array(mem_ptr_)
      {
          if (!isTypeMatch<T>())
              throw PixelTypeException(m_pixelType, "TypedImage asked to create a container for PixelType with inconsitent type.");
      }

      /*!
       * @brief the [] accessor, for accessing or changing a pixel value
       * @param idxs_xy_ The X, Y coordinate in the plane (or X, Y, C} order if 3D. can be provided as an initializer list {x, y, c}
       * @return a reference to the pixel
       */
      T& operator[](const std::vector<size_t>& idxs_xy_);

      /*!
       * @brief an alternate accessor to the pixel value in CYX order
       * @param idxs_cyx_ a vector or initializer list of C,Y,X indices
       * @return a reference to the pixel
       */
      T& getCYX(std::vector<size_t> idxs_cyx_) { return (*this)[std::vector<size_t>(idxs_cyx_.rbegin(), idxs_cyx_.rend())]; }

      /*!
       * @brief return the raw_pointer to the memory the image class contains, be careful with raw pointer manipulation. here be segfaults
       * @param jump_to_ an integer offset from the beginning of the array.
       * @return a pointer to the internally managed memory. Image maintains ownership!
       */
      T* getRawPtr(size_t jump_to_ = 0) { return m_array+jump_to_; }

      /*!
       * return a pointer to the specified memory poisiton
       * @param list_ a list of coordinates consistent with the internal storage
       * @return A pointer into the raw internal data (Image still maintains ownership of the memory).
       */
      T* getRawPtr(std::vector<size_t> list_); // inline definititon below

      /*!
       * @brief Copy the image from the libCZI bitmap object into this Image object
       * @param bitmap_ptr_ is the image bitmap from libCZI
       * @param channels_ the number of channels 1 for GrayX, 3 for BgrX etc. (ie the number of XY planes required to hold the image)
       */
      void loadImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_, size_t channels_) override;

      /*!
       * @brief return pointers to begin, end for the specific channel in a 3 channel image
       * @param channel_ to select from the image {0, 1, or 2}
       * @return a pair of pointers corresponding to begin and end for the channel specified
       */
      std::pair<T*, T*> channelPtrs(int channel_);

// TODO Implement set_sort_order() and operator()<
  };

  template<typename T>
  inline T& TypedImage<T>::operator[](const std::vector<size_t>& idxs_xy_)
  {
      if (idxs_xy_.size()!=m_shape.size())
          throw ImageAccessUnderspecifiedException(idxs_xy_.size(), m_shape.size(), "from TypedImage.operator[].");
      size_t idx = calculateIdx(idxs_xy_);
      return m_array[idx];
  }

  template<typename T>
  inline T* TypedImage<T>::getRawPtr(std::vector<size_t> list_)
  {
      std::vector<size_t> zeroPadded(0, m_shape.size());
      std::copy(list_.rbegin(), list_.rend(), zeroPadded.rbegin());
      return this->operator[](calculateIdx(zeroPadded));
  }
  template<typename T>
  inline void TypedImage<T>::loadImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_, size_t channels_)
  {
      libCZI::IntSize size = bitmap_ptr_->GetSize();
      {
          libCZI::ScopedBitmapLockerP lckScoped{bitmap_ptr_.get()};
          // WARNING do not compute the end of the array by multiplying stride by height, they are both uint32_t and you'll get an overflow for larger images
          uint8_t* sEnd = static_cast<uint8_t*>(lckScoped.ptrDataRoi)+lckScoped.size;
          SourceRange<T> sourceRange(channels_, static_cast<T*>(lckScoped.ptrDataRoi), (T*) (sEnd), lckScoped.stride, size.w);
          TargetRange<T> targetRange(channels_, size.w, size.h, m_array, m_array+length());
          for (std::uint32_t h = 0; h<bitmap_ptr_->GetHeight(); ++h) {
              pairedForEach(sourceRange.strideBegin(h), sourceRange.strideEnd(h), targetRange.strideBegin(h),
                  [&](std::vector<T*> src_, std::vector<T*> tgt_) {
                      pairedForEach(src_.begin(), src_.end(), tgt_.begin(), [&](T* s_, T* t_) {
                          *t_ = *s_;
                      });
                  });
          }
      }
  }

  // TODO: I need to think about how I'm handling BGR images and the memory. I think splitting them off the bat is the
  // TODO: right thing to do but I'll likely have to come back and fix some of this.
  template<typename T>
  inline std::pair<T*, T*>
  TypedImage<T>::channelPtrs(int channel_)
  {
      std::pair<T*, T*> ans;
      size_t planeSize = std::accumulate(m_shape.rbegin(), --(m_shape.rend()), 1, std::multiplies<size_t>());
      ans.first = m_array+channel_*planeSize;
      ans.second = ans.first+planeSize;
      return ans;
  }

}
#endif //_PYLIBCZI_TYPEDIMAGE_H
