//
// Created by Jamie Sherman on 2019-08-28.
//

#ifndef _PYLIBCZI__PYLIBCZI_IMAGE_H
#define _PYLIBCZI__PYLIBCZI_IMAGE_H

#include <cstdio>
#include <cstdlib>
#include <array>
#include <map>
#include <utility>
#include <vector>
#include <utility>
#include <functional>
#include "exceptions.h"
#include <libCZI/libCZI_Pixels.h>

namespace pylibczi {

    // forward declare for use in casting in ImageBC
    template<typename T>
    class Image;
    class ImageFactory;
    class citerator;

    class ImageBC {
    protected:
      std::vector<size_t> m_matrixSizes;
      libCZI::PixelType m_pixelType;
      static std::unique_ptr<std::map<libCZI::PixelType, std::string> > m_pixelToTypeName;


      size_t calculate_idx(const std::vector<size_t> &idxs);
    public:
      ImageBC(std::vector<size_t> shp, libCZI::PixelType pt)
          : m_matrixSizes(std::move(shp)), m_pixelType(pt) {}

      template<typename T>
      std::shared_ptr<Image<T> > get_derived();

      template<typename T>
      bool is_type_match();


      size_t length() {return std::accumulate(m_matrixSizes.begin(), m_matrixSizes.end(), 1, std::multiplies<>());}
      size_t n_of_channels();
      size_t size_of_pixel_type(libCZI::PixelType pt);

      virtual void load_image(const std::shared_ptr<libCZI::IBitmapData> &pBitmap, size_t channels) = 0;
    };

    template<typename T>
    class Image : public ImageBC {
      std::unique_ptr<T> m_array;

      // allow ImageFactory access -> 2 statements below mean ImageFactory is the only way to make an image
      // this prevents people from mucking up the order indexing of image to memory copying
      friend ImageFactory;

      // private constructor
      Image(std::vector<size_t> shp, libCZI::PixelType pt) : ImageBC(shp, pt), m_array() {}
    public:

      T &operator[](std::vector<size_t> idxsXY);

      T *get_raw_ptr(int jumpTo = 0) { return m_array + jumpTo; }
      T *get_raw_ptr(std::vector<size_t> lst);

      void load_image(const std::shared_ptr<libCZI::IBitmapData> &pBitmap, size_t channels) override;
    };

    template<typename T>
    class TargetRange {
      const size_t m_width;
      const size_t m_height;
      T* m_begin;
      T* m_end;
      size_t area() {return m_width*m_height;}
    public:
      TargetRange(size_t w, size_t h, T* _begin, T* _end): m_width(w), m_height(h), m_begin(_begin), m_end(_end) {}
      void addPixels(size_t offset) { m_begin += offset; }

      class target_3channel_iterator {
        std::array<T*, 3> m_ptr;
      public:
        target_3channel_iterator(T* ps, size_t wh) :m_ptr{ps, ps+wh, ps+wh*2} {}
        target_3channel_iterator& operator++() { std::for_each(m_ptr.begin(), m_ptr.end(), [](T*& p){++p;}); return *this;}
        target_3channel_iterator operator++(int) {target_3channel_iterator retval = *this; ++(*this); return retval;}
        bool operator==(target_3channel_iterator other) const {return m_ptr.begin() == other.m_ptr.begin();}
        bool operator!=(target_3channel_iterator other) const {return !(*this == other);}
        std::tuple<T&, T&, T&> operator*() {return std::tuple<T&, T&, T&>{*(m_ptr[0]),*(m_ptr[1]),*(m_ptr[2])};}
        // iterator traits
        using difference_type = size_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;
      };
      target_3channel_iterator begin() {return target_3channel_iterator(m_begin, area());}
      target_3channel_iterator stride_begin(size_t h) {return target_3channel_iterator(m_begin+h*m_width, area());}
      target_3channel_iterator end() {return target_3channel_iterator(m_end-2*area(), area());}
    };

    template<typename T>
    class SourceRange {
    public:
      T* m_begin;
      T* m_end;
      size_t m_stride;
      size_t m_pixels_per_stride;
      SourceRange(T* _begin, T* _end, size_t _stride, size_t pxls_per_stride)
      : m_begin(_begin), m_end(_end), m_stride(_stride), m_pixels_per_stride(pxls_per_stride) {}
      class source_3channel_iterator {
        std::array<T*,3> m_ptr;
      public:
        explicit source_3channel_iterator(T* ptr): m_ptr{ptr, ptr+1, ptr+2} {}
        source_3channel_iterator& operator++() { std::for_each(m_ptr.begin(), m_ptr.end(), [](T*& p){++p; ++p; ++p;}); return *this;}
        source_3channel_iterator operator++(int) {source_3channel_iterator retval = *this; ++(*this); return retval;}
        bool operator==(const source_3channel_iterator &other) const {return *(m_ptr.begin()) == *(other.m_ptr.begin());}
        bool operator!=(const source_3channel_iterator &other) const {return !(*this == other);}
        std::tuple<T&,T&,T&> operator*() {return std::tuple<T&, T&, T&>(*(m_ptr[0]),*(m_ptr[1]),*(m_ptr[2]));}
        // iterator traits
        using difference_type = size_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;
      };
      source_3channel_iterator begin() {return source_3channel_iterator(m_begin);}
      source_3channel_iterator stride_begin(size_t h){ return source_3channel_iterator((T*)(((uint8_t *)m_begin) + h*m_stride)); }
      source_3channel_iterator stride_end(size_t h) {
          auto tmp = (uint8_t *)m_begin; tmp += h*m_stride + m_pixels_per_stride*3; m_begin = (T*)tmp;
          T* send = (T*)tmp;
          if(send > m_end) throw ImageIteratorException("stride advanced pointer beyond end of array.");
          return source_3channel_iterator(send);
      }
      source_3channel_iterator end() {return source_3channel_iterator(m_end);}
    };

    class ImageFactory{
      using PT = libCZI::PixelType;
      using V_ST = std::vector<size_t>;
      using ConstrMap = std::map< libCZI::PixelType, std::function<std::shared_ptr<ImageBC>(std::vector<size_t>, libCZI::PixelType pt) > >;
      static ConstrMap m_pixelToImage;
    public:
      static size_t size_of_pixel_type(PT pt);
      static size_t n_of_channels(PT pt);
      std::shared_ptr<ImageBC> construct_image(const std::shared_ptr<libCZI::IBitmapData> &pBitmap);
    };
}

#endif //_PYLIBCZI__PYLIBCZI_IMAGE_H
