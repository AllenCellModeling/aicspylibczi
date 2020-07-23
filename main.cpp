//
// This is a test program fro profiling performance
//

#include <cstdio>
#include <cstdlib>

#include "_aicspylibczi/Reader.h"

int
main(void){
    pylibczi::Reader czi(L"/Users/jamies/Data/s_1_t_10_c_3_z_1.czi");

    libCZI::CDimCoordinate dm = libCZI::CDimCoordinate{{libCZI::DimensionIndex::B, 0},
                                                       {libCZI::DimensionIndex::S, 0}};

    std::cout << "Dims: " << czi.dimsString() << std::endl;
    auto dSizes = czi.dimSizes();

    std::cout << "Shape: {" ;
    for_each(dSizes.begin(), dSizes.end(), [](const int &x){
        std::cout << x << ", ";
    });
    std::cout << "}" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    auto pr = czi.readSelected(dm);
    auto done = std::chrono::high_resolution_clock::now();


    std::cout << "Duration(milliseconds): " << std::chrono::duration_cast<std::chrono::milliseconds>(done-start).count();

    return 0;
}
