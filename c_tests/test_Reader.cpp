//
// Created by James Sherman on 9/6/19.
//

#include "catch.hpp"
#include "../_pylibczi/Reader.h"
#include <unistd.h>

TEST_CASE("test_reader_constructor", "make sure constructor doesn't throw an exception."){
    char buff[200];
    std::cout << getcwd(buff, 200) << std::endl;
    FILE *fp = std::fopen("../../pylibczi/tests/resources/s_1_t_1_c_1_z_1.czi", "rb");      // #include <cstdio>
    if(fp == nullptr) std::cout << "failed to open file!" << std::endl;
    REQUIRE_NOTHROW(pylibczi::Reader(fp));
}
