#ifndef _PYLIBCZI_SOURCERANGE_H
#define _PYLIBCZI_SOURCERANGE_H

#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>

#include "exceptions.h"

namespace pylibczi {

template<typename T>
class SourceRange
{
  T* m_begin;
  T* m_end;
  size_t m_stride;
  size_t m_pixelsPerStride;
  size_t m_channels;

public:
  SourceRange(size_t channels_, T* begin_, T* end_, size_t stride_, size_t pixels_per_stride_)
    : m_channels(channels_)
    , m_begin(begin_)
    , m_end(end_)
    , m_stride(stride_)
    , m_pixelsPerStride(pixels_per_stride_)
  {}

  class SourceChannelIterator
  {
    std::vector<T*> m_channelIterators;

  public:
    SourceChannelIterator(size_t number_of_channels_, T* ptr_)
      : m_channelIterators(number_of_channels_)
    {
      std::generate(m_channelIterators.begin(), m_channelIterators.end(), [ptr_]() mutable { return ptr_++; });
    }

    SourceChannelIterator& operator++()
    {
      size_t numberOfChannels = m_channelIterators.size();
      std::for_each(m_channelIterators.begin(), m_channelIterators.end(), [numberOfChannels](T*& p_) {
        p_ = p_ + numberOfChannels;
      });
      return *this;
    }

    SourceChannelIterator operator++(int)
    {
      SourceChannelIterator preIncrementIterator = *this;
      ++(*this);
      return preIncrementIterator;
    }

    bool operator==(const SourceChannelIterator& other_) const
    {
      return *(m_channelIterators.begin()) == *(other_.m_channelIterators.begin());
    }

    bool operator!=(const SourceChannelIterator& other_) const { return !(*this == other_); }

    std::vector<T*>& operator*() { return m_channelIterators; }
    // iterator traits
    using difference_type = size_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::forward_iterator_tag;
  };

  SourceChannelIterator begin() { return SourceChannelIterator(m_channels, m_begin); }

  SourceChannelIterator strideBegin(size_t y_index_)
  {
    return SourceChannelIterator(m_channels, (T*)(((uint8_t*)m_begin) + y_index_ * m_stride));
  }

  SourceChannelIterator strideEnd(size_t y_index_)
  {
    auto tmp = (uint8_t*)m_begin;
    tmp += y_index_ * m_stride + m_pixelsPerStride * m_channels * sizeof(T);
    T* sEnd = (T*)tmp;
    if (sEnd > m_end)
      throw ImageIteratorException("stride advanced pointer beyond end of array.");
    return SourceChannelIterator(m_channels, sEnd);
  }

  SourceChannelIterator end() { return SourceChannelIterator(m_channels, m_end); }
};
}
#endif //_PYLIBCZI_SOURCERANGE_H
