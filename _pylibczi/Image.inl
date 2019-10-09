
namespace pylibczi {


  template<typename T>
  inline std::shared_ptr<Image<T> > get_derived(std::shared_ptr<ImageBC> ptr)
{
  if (!ptr->is_type_match<T>())
  throw PixelTypeException(ptr->pixelType(), "Image PixelType doesn't match requested memory type.");
  return std::dynamic_pointer_cast<Image<T> >(ptr);
}

  template<typename T>
  inline ImageBC::ImVec Image<T>::split_channels(int startFrom)
  {
      ImVec ivec;
      if (m_matrixSizes.size()<3)
          throw ImageSplitChannelException("Image  only has 2 dimensions. No channels to split.", 0);
      int cStart = 0;
      // TODO figure out if C can have a nonzero value for a BGR image
      if (m_cdims.TryGetPosition(libCZI::DimensionIndex::C, &cStart) && cStart!=0)
          throw ImageSplitChannelException("attempting to split channels", cStart);
      for (int i = 0; i<m_matrixSizes[0]; i++) {
          libCZI::CDimCoordinate tmp(m_cdims);
          tmp.Set(libCZI::DimensionIndex::C, i+startFrom); // assign the channel from the BGR
          // TODO should I change the pixel type from a BGRx to a Grayx/3
          ivec.emplace_back(new Image<T>({m_matrixSizes[1], m_matrixSizes[2]}, m_pixelType, &tmp, m_xywh, m_mIndex));
      }
      return ivec;
  }

  template<typename T>
  inline T& Image<T>::operator[](const std::vector<size_t>& idxs)
  {
      if (idxs.size()!=m_matrixSizes.size())
          throw ImageAccessUnderspecifiedException(idxs.size(), m_matrixSizes.size(), "from Image.operator[].");
      size_t idx = calculate_idx(idxs);
      return m_array[idx];
  }

  template<typename T>
  inline T* Image<T>::get_raw_ptr(std::vector<size_t> lst)
  {
      std::vector<size_t> zeroPadded(0, m_matrixSizes.size());
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

}