//
// Created by Jamie Sherman on 2019-08-28.
//

#include <algorithm>
#include <numeric>
#include <typeinfo>
#include <utility>

#include "Image.h"
#include "Iterator.h"
#include "exceptions.h"
#include "helper_algorithms.h"

namespace pylibczi {

    std::unique_ptr<std::map<libCZI::PixelType, std::string> > ImageBC::m_pixelToTypeName(
        new std::map<libCZI::PixelType, std::string>{
            {libCZI::PixelType::Gray8, typeid(uint8_t).name()},        // 8-bit grayscale
            {libCZI::PixelType::Gray16, typeid(uint16_t).name()},       // 16-bit grayscale
            {libCZI::PixelType::Gray32Float, typeid(float).name()},          // 4-byte float
            {libCZI::PixelType::Bgr24, typeid(uint8_t).name()},        // 8-bit triples (order B, G, R).
            {libCZI::PixelType::Bgr48, typeid(uint16_t).name()},       // 16-bit triples (order B, G, R).
            {libCZI::PixelType::Bgr96Float, typeid(float).name()},          // 4-byte triples (order B, G, R).
            {libCZI::PixelType::Bgra32, typeid(nullptr).name()},    // unsupported by libCZI
            {libCZI::PixelType::Gray64ComplexFloat, typeid(nullptr).name()},    // unsupported by libCZI
            {libCZI::PixelType::Bgr192ComplexFloat, typeid(nullptr).name()},    // unsupported by libCZI
            {libCZI::PixelType::Gray32, typeid(nullptr).name()},    // unsupported by libCZI
            {libCZI::PixelType::Gray64Float, typeid(nullptr).name()}     // unsupported by libCZI
        });

    size_t ImageBC::calculate_idx(const std::vector<size_t> &idxs) {
        if (idxs.size() != m_matrixSizes.size())
            throw ImageAccessUnderspecifiedException(idxs.size(), m_matrixSizes.size(), "Sizes must match");
        size_t running_product = 1;
        std::vector<size_t> weights(1, 1);
        std::for_each(m_matrixSizes.begin() + 1, m_matrixSizes.end(), [&weights, &running_product](const size_t len) {
          running_product *= len;
          weights.emplace_back(running_product);
        });
        std::vector<size_t> prod(m_matrixSizes.size(), 0);
        std::transform(idxs.begin(), idxs.end(), weights.begin(), prod.begin(), [](size_t a, size_t b) -> size_t {
          return a * b;
        });
        size_t idx = std::accumulate(prod.begin(), prod.end(), size_t(0));
        return idx;
    }

    template<typename T>
    bool
    ImageBC::is_type_match() {
        return (typeid(T).name() == (*m_pixelToTypeName)[m_pixelType]);
    }

    template<typename T>
    std::shared_ptr<Image<T> >
    ImageBC::get_derived() {
        if (!is_type_match<T>())
            throw PixelTypeException(m_pixelType, "Image PixelType doesn't match requested memory type.");
        return std::shared_ptr<Image<T> >(dynamic_cast< Image<T> *>(this));
    }

    template<typename T>
    T &Image<T>::operator[](const std::vector<size_t>& idxs) {
        if (idxs.size() != m_matrixSizes.size())
            throw ImageAccessUnderspecifiedException(idxs.size(), m_matrixSizes.size(), "from Image.operator[].");
        size_t idx = calculate_idx(idxs);
        return m_array[idx];
    }

    template<typename T>
    T *Image<T>::get_raw_ptr(std::vector<size_t> lst) {
        std::vector<size_t> zeroPadded(0, m_matrixSizes.size());
        std::copy(lst.rbegin(), lst.rend(), zeroPadded.rbegin());
        return this->operator[](calculate_idx(zeroPadded));
    }
    template<typename T>
    void Image<T>::load_image(const std::shared_ptr<libCZI::IBitmapData> &pBitmap, size_t channels) {
        libCZI::IntSize size = pBitmap->GetSize();
        {
            libCZI::ScopedBitmapLockerP lckScoped{pBitmap.get()};

// TODO either put in a case statement or addapt the iterators to take the number of channels
            auto sEnd = static_cast<uint8_t *>(lckScoped.ptrDataRoi) + size.h*lckScoped.stride;
            SourceRange<T> sourceRange(channels, static_cast<T *>(lckScoped.ptrDataRoi), (T *)(sEnd),lckScoped.stride, size.w);
            TargetRange<T> targetRange(channels, size.w, size.h, m_array.get(), m_array.get() + length());
            for (std::uint32_t h = 0; h < pBitmap->GetHeight(); ++h) {
               paired_for_each(sourceRange.stride_begin(h), sourceRange.stride_end(h), targetRange.stride_begin(h),
               [](std::vector<T*> src, std::vector<T*>tgt){
                   paired_for_each(src.begin(), src.end(), tgt.begin(), [](T* s, T* t){
                       *t = *s;
                   });
               });
            }
        }
    }

    ImageFactory::ConstrMap ImageFactory::m_pixelToImage{
        {PT::Gray8, [](V_ST shp, PT pt) { return std::shared_ptr<Image<uint8_t> >(new Image<uint8_t>( std::move(shp), pt)); }},
        {PT::Bgr24, [](V_ST shp, PT pt) { return std::shared_ptr<Image<uint8_t> >(new Image<uint8_t>( std::move(shp), pt)); }},
        {PT::Gray16, [](V_ST shp, PT pt) { return std::shared_ptr<Image<uint16_t> >(new Image< uint16_t>( std::move(shp), pt)); }},
        {PT::Bgr48, [](V_ST shp, PT pt) { return std::shared_ptr<Image<uint16_t> >(new Image< uint16_t>( std::move(shp), pt)); }},
        {PT::Gray32Float, [](V_ST shp, PT pt) { return std::shared_ptr<Image<float> >(new Image<float>( std::move(shp), pt)); }},
        {PT::Bgr96Float, [](V_ST shp, PT pt) { return std::shared_ptr<Image<float> >(new Image<float>( std::move(shp), pt)); }}
    };

    size_t
    ImageFactory::size_of_pixel_type(PT pt) {
        switch (pt) {
        case PT::Gray8:
        case PT::Bgr24:return sizeof(uint8_t);
        case PT::Gray16:
        case PT::Bgr48:return sizeof(uint16_t);
        case PT::Gray32Float:
        case PT::Bgr96Float:return sizeof(float);
        default:throw PixelTypeException(pt, "Pixel Type unsupported by libCZI.");
        }
    }

    size_t
    ImageFactory::n_of_channels(libCZI::PixelType pt) {
        using PT = libCZI::PixelType;
        switch (pt) {
        case PT::Gray8:
        case PT::Gray16:
        case PT::Gray32Float:return 1;
        case PT::Bgr24:
        case PT::Bgr48:
        case PT::Bgr96Float:return 3;
        default:throw PixelTypeException(pt, "Pixel Type unsupported by libCZI.");
        }

    };

    std::shared_ptr<ImageBC> ImageFactory::construct_image(const std::shared_ptr<libCZI::IBitmapData> &pBitmap) {
        libCZI::IntSize size = pBitmap->GetSize();
        libCZI::PixelType pt = pBitmap->GetPixelType();

        std::vector<size_t> shp;
        size_t channels = n_of_channels(pt);
        if (channels == 3)
            shp.emplace_back(3);
        shp.emplace_back(size.h);
        shp.emplace_back(size.w);

        std::shared_ptr<ImageBC> img = m_pixelToImage[pt](shp, pt);
        img->load_image(pBitmap, channels);

        return std::shared_ptr<ImageBC>(img);
    }

}
