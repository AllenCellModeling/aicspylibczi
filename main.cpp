//
// This is a test program fro profiling performance
//

#include <cstdio>
#include <cstdlib>
#include <locale>
#include <codecvt>
#include <string>

#include "_aicspylibczi/Reader.h"

int
main(int argc, char *argv[]){
    if(argc != 2){
        std::cout << "This executable is intended for profiling. It requires the user provide the path to a large czi file." << std::endl;\
        std::cout << "Example Usage: profile_me /Users/jamies/Data/s_1_t_10_c_3_z_1.czi" << std::endl;
        std::cerr << "Program called with insufficient or excess arguments!" << std::endl;
        exit(0);
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(argv[1]);

    pylibczi::Reader czi( wide.c_str() ); // L"/Users/jamies/Data/s_1_t_10_c_3_z_1.czi");

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
