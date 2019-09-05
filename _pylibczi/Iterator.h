//
// Created by James Sherman on 9/2/19.
//

#ifndef _PYLIBCZI_ITERATOR_H
#define _PYLIBCZI_ITERATOR_H

#include "exceptions.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>

namespace pylibczi {


    template<typename T>
    class SourceRange {
        T *m_begin;
        T *m_end;
        size_t m_stride;
        size_t m_pixels_per_stride;
        size_t m_channels;

    public:
        SourceRange(size_t _channels, T *_begin, T *_end, size_t _stride, size_t pxls_per_stride)
                : m_channels(_channels), m_begin(_begin), m_end(_end), m_stride(_stride),
                  m_pixels_per_stride(pxls_per_stride) {}

        class source_channel_iterator {
            std::vector<T *> m_ptr;

        public:
            source_channel_iterator(size_t ch, T *ptr) : m_ptr(ch) {
                std::generate(m_ptr.begin(), m_ptr.end(), [ptr]() mutable { return ptr++; });
            }

            source_channel_iterator &operator++() {
                size_t n_of_c = m_ptr.size();
                std::for_each(m_ptr.begin(), m_ptr.end(),
                              [n_of_c](T *&p) { p = p + n_of_c; });
                return *this;
            }

            source_channel_iterator operator++(int) {
                source_channel_iterator retval = *this;
                ++(*this);
                return retval;
            }

            bool operator==(const source_channel_iterator &other) const {
                return *(m_ptr.begin()) == *(other.m_ptr.begin());
            }

            bool operator!=(const source_channel_iterator &other) const {
                return !(*this == other);
            }

            std::vector<T*> operator*() {
                return m_ptr;
            }
            // iterator traits
            using difference_type = size_t;
            using value_type = T ;
            using pointer = T*;
            using reference = T&;
            using iterator_category = std::forward_iterator_tag;
        };

        source_channel_iterator begin() {
            return source_channel_iterator(m_channels, m_begin);
        }

        source_channel_iterator stride_begin(size_t h) {
            return source_channel_iterator(m_channels, (T *) (((uint8_t *) m_begin) + h * m_stride));
        }

        source_channel_iterator stride_end(size_t h) {
            auto tmp = (uint8_t *) m_begin;
            tmp += h * m_stride + m_pixels_per_stride * m_channels;
            T *send = (T *) tmp;
            if (send > m_end)
                throw ImageIteratorException(
                        "stride advanced pointer beyond end of array.");
            return source_channel_iterator(m_channels, send);
        }

        source_channel_iterator end() { return source_channel_iterator(m_channels, m_end); }
    };

    template<typename T>
    class TargetRange {
        const size_t m_channels;
        const size_t m_width;
        const size_t m_height;
        T *m_begin;
        T *m_end;

        size_t area() { return m_width * m_height; }

    public:
        TargetRange(size_t channels, size_t w, size_t h, T *_begin, T *_end)
                : m_channels(channels), m_width(w), m_height(h), m_begin(_begin), m_end(_end) {}

        void addPixels(size_t offset) { m_begin += offset; }

        class target_channel_iterator {
            std::vector<T *> m_ptr;

        public:
            target_channel_iterator(size_t ch, T *ps, size_t wh)
                    : m_ptr(ch) {
                size_t h = 0;
                std::generate(m_ptr.begin(), m_ptr.end(), [ps, h, wh]() mutable {
                    h++;
                    return ps + wh * (h - 1);
                });
            }

            target_channel_iterator &operator++() {
                std::for_each(m_ptr.begin(), m_ptr.end(), [](T *&p) { ++p; });
                return *this;
            }

            target_channel_iterator operator++(int) {
                target_channel_iterator retval = *this;
                ++(*this);
                return retval;
            }

            bool operator==(target_channel_iterator other) const {
                return m_ptr.begin() == other.m_ptr.begin();
            }

            bool operator!=(target_channel_iterator other) const {
                return !(*this == other);
            }

            std::vector<T*> operator*() {
                return m_ptr;
            }
            // iterator traits
            using difference_type = size_t;
            using value_type = std::vector<T>;
            using pointer = std::vector<T*>;
            using reference = std::vector<T&>;
            using iterator_category = std::forward_iterator_tag;
        };

        target_channel_iterator begin() {
            return target_channel_iterator(m_channels, m_begin, area());
        }

        target_channel_iterator stride_begin(size_t h) {
            return target_channel_iterator(m_channels, m_begin + h * m_width, area());
        }

        target_channel_iterator end() {
            return target_channel_iterator(m_channels, m_end - 2 * area(), area());
        }
    };


}
#endif //_PYLIBCZI_ITERATOR_H
