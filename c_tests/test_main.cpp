#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <pybind11/embed.h>

namespace py=pybind11;

py::scoped_interpreter guard{};

TEST_CASE("cast_test", "Is FILE * default constructible")
{
    REQUIRE(std::is_default_constructible<FILE*>());
    // REQUIRE(std::is_default_constructible<std::istream>()); // istream is NOT default constructible/**/!
}
