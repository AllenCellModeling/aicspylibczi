cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MACOSX_RPATH 1)
project(_pylibczi)
# build problems with library dependencies if I go above C++14
# namely the dependencies on the Eigen library and they have to update
# their use of std::bind2nd / std::bind1st to std::bind
#set(CMAKE_CXX_COMPILER /Applications/CLion.app/Contents/bin/clang)


set(CMAKE_CXX_STANDARD 14)
set(PYBIND11_CPP_STANDARD -std=c++14)

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # linking with 'thread' is necessary if we use std::thread and related under Linux it seems
    #   otherwise - the program simply crashes (no build-error)
    set(CMAKE_C_COMPILER /usr/bin/clang)
    set(CMAKE_CXX_COMPILER /usr/bin/clang++)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14 -fPIC -O0 -g -D_FILE_OFFSET_BITS=64 -fvisibility=hidden")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__ANSI__ -fPIC -D_FILE_OFFSET_BITS=64")
    add_compile_definitions(LINUXENV)
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF (WIN32)

ENDIF (WIN32)

add_compile_definitions(_STATICLIBBUILD)
add_subdirectory(libCZI/Src/JxrDecode)
add_subdirectory(libCZI/Src/libCZI)
add_subdirectory(pybind11)

include_directories(libCZI/Src)
find_library(libCZI REQUIRED PATHS ${CMAKE_SOURCE_DIR}/libCZI)

set(PYLIBCZI_C_SRC_HEADERS _pylibczi/Reader.h _pylibczi/helper_algorithms.h _pylibczi/exceptions.h _pylibczi/inc_libCZI.h
        _pylibczi/IndexMap.h _pylibczi/Image.h _pylibczi/pylibczi_ostream.h)

set(PYLIBCZI_C_SRC _pylibczi/Reader.cpp _pylibczi/IndexMap.cpp _pylibczi/Image.cpp _pylibczi/pylibczi_ostream.cpp _pylibczi/exceptions.cpp)

set(PYLIBCZI_PYBIND11 _pylibczi/pb_bindings.cpp _pylibczi/pb_helpers.h _pylibczi/pb_helpers.cpp _pylibczi/pb_caster_ImageVector.h
        _pylibczi/pb_caster_BytesIO.h _pylibczi/pb_caster_libCZI_DimensionIndex.h)

set(TARGET_ONE libczi_c++_extension)
set(TARGET_TWO _pylibczi)

set_target_properties(libCZI PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
add_library(${TARGET_ONE} STATIC ${PYLIBCZI_C_SRC} ${PYLIBCZI_C_SRC_HEADERS})
target_link_libraries(${TARGET_ONE} INTERFACE libCZIStatic JxrDecodeStatic)
add_dependencies(${TARGET_ONE} libCZIStatic JxrDecodeStatic)

pybind11_add_module(${TARGET_TWO} MODULE ${PYLIBCZI_C_SRC} ${PYLIBCZI_PYBIND11})
target_link_libraries(${TARGET_TWO} PRIVATE libCZIStatic JxrDecodeStatic)
add_custom_command(TARGET ${TARGET_TWO} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_TWO}> ${PROJECT_SOURCE_DIR}
        )

add_subdirectory(c_tests)


