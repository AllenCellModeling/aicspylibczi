//
// Created by Jamie Sherman on 2019-09-11.
//

#ifndef _PYLIBCZI__PYLIBCZI_PB_HELPERS_H
#define _PYLIBCZI__PYLIBCZI_PB_HELPERS_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>

namespace py=pybind11;
namespace pb_helpers{

    template <typename T>
    std::unique_ptr< py::array_t<T> > pack_array(std::vector<ptrdiff_t> shp, T* ptr){
        py::array_t<T> *ans;
        ans = new py::array_t<T>(shp, ptr);
        return std::unique_ptr< py::array_t<T> >(ans);
    }

}



#endif //_PYLIBCZI__PYLIBCZI_PB_HELPERS_H
