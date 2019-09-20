//
// Created by James Sherman on 9/6/19.
//

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <pybind11/embed.h>

namespace py=pybind11;

py::scoped_interpreter guard{};

TEST_CASE( "sanity_check", "Prove that 1 equals 1" ){
int one = 1;
REQUIRE( one == 1 );
}

TEST_CASE( "cast_test", "Is FILE * default constructible"){
	REQUIRE(std::is_default_constructible<FILE*>());
	// REQUIRE(std::is_default_constructible<std::istream>()); // istream is NOT default constructible!
}