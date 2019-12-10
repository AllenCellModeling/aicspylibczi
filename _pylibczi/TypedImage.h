#ifndef _PYLIBCZI_TYPEDIMAGE_H
#define _PYLIBCZI_TYPEDIMAGE_H

#include "Image.h"
#include "SourceRange.h"
#include "TargetRange.h"

namespace pylibczi {

  template<typename T>
  class TypedImage: public Image {
      std::unique_ptr<T[]> m_array;

  public:
      /*!
       * @brief The Image constructor creates the container and memory for storing an image from a ZeissRaw/CZI file. This class
       * is really intended to be created by ImageFactory.
       * @param shape_ The shape of the image can be in a vector or a tuple but <b>MUST be in {C, Y, X} order</b>.
       * @param pixel_type_ The Pixel type of the image,
       * @param plane_coordantes_ The coordinate structure used to define the plane, what scene, channel, time-point etc.
       * @param box_ The (x0, y0, w, h) structure containing the logical position of the image.
       * @param m_index_ The mosaic index for the image, this is only relevant if the file is a mosaic file.
       */
      TypedImage(std::vector<size_t> shape_, libCZI::PixelType pixel_type_, const libCZI::CDimCoordinate* plane_coordantes_,
          libCZI::IntRect box_, int m_index_)
          :Image(shape_, pixel_type_, plane_coordantes_, box_, m_index_),
           m_array(new T[std::accumulate(shape_.begin(), shape_.end(), (size_t) 1, std::multiplies<>())])
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
      T* getRawPtr(int jump_to_ = 0) { return m_array.get()+jump_to_; }

      /*!
       * return a pointer to the specified memory poisiton
       * @param list_ a list of coordinates consistent with the internal storage
       * @return A pointer into the raw internal data (Image still maintains ownership of the memory).
       */
      T* getRawPtr(std::vector<size_t> list_); // inline definititon below

      /*!
       * @brief This function releases the memory from the container and gives it to the recipient to handle. The recipient takes
       * responsible for freeing the memory.
       * @return The raw pointer of type T*, where T is the storage type corresponding with the PixelType
       */
      T* releaseMemory()
      {
          if (!isTypeMatch<T>())
              throw PixelTypeException(pixelType(), "TypedImage PixelType is inconsistent with requested memory type.");
          return m_array.release();
      }

      /*!
       * @brief Copy the image from the libCZI bitmap object into this Image object
       * @param bitmap_ptr_ is the image bitmap from libCZI
       * @param channels_ the number of channels 1 for GrayX, 3 for BgrX etc. (ie the number of XY planes required to hold the image)
       */
      void loadImage(const std::shared_ptr<libCZI::IBitmapData>& bitmap_ptr_, size_t channels_) override;

      /*!
       * @brief If this container is a 3channel BGR image split it into single channel images so they can be merged into an data matrix
       * @param start_from_ is an integer offset to start assigning the new channels from.
       * @return a vector of smart pointers wrapping Images (2D)
       */
      ImVec splitChannels(int start_from_) override;
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
          TargetRange<T> targetRange(channels_, size.w, size.h, m_array.get(), m_array.get()+length());
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

  template<typename T>
  inline Image::ImVec TypedImage<T>::splitChannels(int start_from_)
  {
      ImVec ivec;
      if (m_shape.size()<3)
          throw ImageSplitChannelException("TypedImage  only has 2 dimensions. No channels to split.", 0);
      int cStart = 0;
      // TODO figure out if C can have a nonzero value for a BGR image
      if (m_planeCoordinate.TryGetPosition(libCZI::DimensionIndex::C, &cStart) && cStart!=0)
          throw ImageSplitChannelException("attempting to split channels", cStart);
      for (int i = 0; i<m_shape[0]; i++) {
          libCZI::CDimCoordinate tmp(m_planeCoordinate);
          tmp.Set(libCZI::DimensionIndex::B, 0);  // to be consistent with other types
          tmp.Set(libCZI::DimensionIndex::C, i+start_from_); // assign the channel from the BGR
          // TODO should I change the pixel type from a BGRx to a Grayx/3
          libCZI::PixelType pt = s_pixelSplitMap[m_pixelType];
          if(pt == libCZI::PixelType::Invalid){
              throw ImageSplitChannelException("Only PixelTypes Bgr24, Bgr48, and Bgr96Float can be split! "
                                               "You have attempted to split an invalid PixelType", cStart);
          }
          ivec.emplace_back(new TypedImage<T>({m_shape[1], m_shape[2]}, pt, &tmp, m_xywh, m_indexM));
      }
      return ivec;
  }

}
#endif //_PYLIBCZI_TYPEDIMAGE_H
