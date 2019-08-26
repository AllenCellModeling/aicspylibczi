cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MACOSX_RPATH 1)
project(_pylibczi)
# build problems with library dependencies if I go above C++14
# namely the dependencies on the Eigen library and they have to update
# their use of std::bind2nd / std::bind1st to std::bind
#set(CMAKE_CXX_COMPILER /Applications/CLion.app/Contents/bin/clang)
set(CMAKE_CXX_STANDARD 14)
set(PYBIND11_CPP_STANDARD -std=c++14)

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        #string(FIND "${_varableName}" "PYTHON" pos1)
        #if(${pos1} GREATER -1)
        message(STATUS "${_variableName}=${${_variableName}}")
        #endif()
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

IF(UNIX)
    # linking with 'thread' is necessary if we use std::thread and related under Linux it seems
    #   otherwise - the program simply crashes (no build-error)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14 -fPIC -D_FILE_OFFSET_BITS=64 -fvisibility=hidden")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__ANSI__ -fPIC -D_FILE_OFFSET_BITS=64")
ENDIF(UNIX)

add_compile_definitions(LINUXENV)
add_subdirectory(libCZI/Src/JxrDecode)
add_subdirectory(libCZI/Src/libCZI)
add_subdirectory(pybind11)

include_directories(libCZI/Src)

find_library(libCZI REQUIRED PATHS ${CMAKE_SOURCE_DIR}/libCZI)

set(PYLIBCZI_C_SRC _pylibczi/aics_added.cpp _pylibczi/IndexMap.cpp)
set(PYLIBCZI_PYBIND11 _pylibczi/pb_bindings.cpp
        _pylibczi/pb_caster_BytesIO.h _pylibczi/pb_caster_libCZI_DimensionIndex.h)

pybind11_add_module(_pylibczi ${PYLIBCZI_C_SRC} ${PYLIBCZI_PYBIND11})
set_target_properties(libCZI PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(_pylibczi PRIVATE libCZI)

print_all_variables()


