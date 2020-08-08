#ifndef _PYLIBCZI_TARGETRANGE_H
#define _PYLIBCZI_TARGETRANGE_H

#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>

#include "exceptions.h"

namespace pylibczi {

  template<typename T>
  class TargetRange {
      const size_t m_channels;
      const size_t m_width;
      const size_t m_height;
      T* m_begin;
      T* m_end;

      size_t area() { return m_width*m_height; }

  public:
      TargetRange(size_t channels_, size_t width_, size_t height_, T* begin_, T* end_)
          :m_channels(channels_), m_width(width_), m_height(height_), m_begin(begin_), m_end(end_) { }

      void addPixels(size_t offset_) { m_begin += offset_; }

      class TargetChannelIterator {
          std::vector<T*> m_ptr;

      public:
          TargetChannelIterator(size_t number_of_channels_, T* ptr_, size_t witdth_times_height_)
              :m_ptr(number_of_channels_)
          {
              size_t h = 0;
              std::generate(m_ptr.begin(), m_ptr.end(), [ptr_, h, witdth_times_height_]() mutable {
                  h++;
                  return ptr_+witdth_times_height_*(h-1);
              });
          }

          TargetChannelIterator& operator++()
          {
              std::for_each(m_ptr.begin(), m_ptr.end(), [](T*& p_) { ++p_; });
              return *this;
          }

          TargetChannelIterator operator++(int)
          {
              TargetChannelIterator retval = *this;
              ++(*this);
              return retval;
          }

          bool operator==(TargetChannelIterator other_) const
          {
              return m_ptr.begin()==other_.m_ptr.begin();
          }

          bool operator!=(TargetChannelIterator other_) const
          {
              return !(*this==other_);
          }

          std::vector<T*> &operator*()
          {
              return m_ptr;
          }
          // iterator traits
          using difference_type = size_t;
          using value_type = std::vector<T>;
          using pointer = std::vector<T*>;
          using reference = std::vector<T&>;
          using iterator_category = std::forward_iterator_tag;
      };

      TargetChannelIterator begin()
      {
          return TargetChannelIterator(m_channels, m_begin, area());
      }

      TargetChannelIterator strideBegin(size_t height_)
      {
          return TargetChannelIterator(m_channels, m_begin+height_*m_width, area());
      }

      TargetChannelIterator end()
      {
          return TargetChannelIterator(m_channels, m_end-2*area(), area());
      }
  };

}
#endif //_PYLIBCZI_TARGETRANGE_H
