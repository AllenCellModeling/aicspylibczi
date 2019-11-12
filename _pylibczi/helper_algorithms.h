#ifndef _PYLIBCZI_HELPER_ALGORITHMS_H
#define _PYLIBCZI_HELPER_ALGORITHMS_H

//! @brief paired_for_each walks through 2 containers in sync. It is similar to the second definition of transform
//! @tparam InputIt An arbitrary iterator type usually associated with a container
//! @tparam OtherIt An arbitrary iterator type usually associated with a container
//! @tparam PairFunction The type of the function f
//! @param first_ The starting iterator of type InputIt
//! @param last_ The ending iterator of type InputIt
//! @param other_first_ The starting iterator of type OtherIt
//! @param f_ A function that takes the values of the de-referenced iterators in order
//! @return the function f
//!
//! @example assign a to b
//! @code
//! std::vector<int> a{0,1,2,3,4,5,6,7,8,9};
//! std::vector<int> b(10, 0);
//! pairedForEach(a.begin(), a.end(), b.begin(), [](int a, int& b){ b = a; });
//! @endcode
template<class InputIt, class OtherIt, class PairFunction>
PairFunction pairedForEach(InputIt first_, InputIt last_, OtherIt other_first_, PairFunction f_)
{
    for (; first_!=last_; ++first_, ++other_first_) {
        f_(*first_, *other_first_);
    }
    return f_;
}

#endif //_PYLIBCZI_HELPER_ALGORITHMS_H
