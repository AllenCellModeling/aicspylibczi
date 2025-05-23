cmake_minimum_required(VERSION 3.10)

set(CMAKE_MACOSX_RPATH 1)

IF (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version" FORCE)
ENDIF(APPLE)

project(_aicspylibczi)
# build problems with library dependencies if I go above C++14
# namely the dependencies on the Eigen library and they have to update
# their use of std::bind2nd / std::bind1st to std::bind
#set(CMAKE_CXX_COMPILER /Applications/CLion.app/Contents/bin/clang)


set(CMAKE_CXX_STANDARD 14)

IF (WIN32)
    set(PYBIND11_CPP_STANDARD /std:c++14)
ELSE()
    # linking with 'thread' is necessary if we use std::thread and related under Linux it seems
    #   otherwise - the program simply crashes (no build-error)
    #set(CMAKE_C_COMPILER clang)
    #set(CMAKE_CXX_COMPILER clang++)
    set(PYBIND11_CPP_STANDARD -std=c++14)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14 -fPIC -O0 -g -D_FILE_OFFSET_BITS=64 -fvisibility=hidden")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__ANSI__ -fPIC -D_FILE_OFFSET_BITS=64")
    add_compile_definitions(LINUXENV)

ENDIF(WIN32)

SET(pybind11_DIR ${CMAKE_CURRENT_LIST_DIR}/pybind11)

add_compile_definitions(_LIBCZISTATICLIB)
add_compile_definitions(LIBCZI_BUILD_UNITTESTS=OFF)
add_compile_definitions(LIBCZI_BUILD_DYNLIB=OFF)
add_compile_definitions(LIBCZI_BUILD_CZICMD=OFF)
add_subdirectory(libCZI)
add_subdirectory(pybind11)

message(STATUS "Found pybind11 v${pybind11_VERSION}: ${pybind11_INCLUDE_DIRS}")

set(PYLIBCZI_C_SRC_HEADERS _aicspylibczi/Reader.h _aicspylibczi/helper_algorithms.h _aicspylibczi/exceptions.h _aicspylibczi/inc_libCZI.h
        _aicspylibczi/IndexMap.h _aicspylibczi/Image.h _aicspylibczi/TypedImage.h _aicspylibczi/ImageFactory.h
        _aicspylibczi/SourceRange.h _aicspylibczi/TargetRange.h _aicspylibczi/pylibczi_ostream.h
        _aicspylibczi/SubblockMetaVec.h _aicspylibczi/DimIndex.h _aicspylibczi/constants.h
        _aicspylibczi/StreamImplLockingRead.h _aicspylibczi/Threadpool.h)

set(PYLIBCZI_C_SRC _aicspylibczi/Reader.cpp _aicspylibczi/IndexMap.cpp _aicspylibczi/Image.cpp _aicspylibczi/ImageFactory.cpp
        _aicspylibczi/pylibczi_ostream.cpp _aicspylibczi/exceptions.cpp _aicspylibczi/SubblockSortable.h
        _aicspylibczi/pb_caster_SubblockMetaVec.h _aicspylibczi/constants.cpp _aicspylibczi/DimIndex.cpp
        _aicspylibczi/StreamImplLockingRead.cpp)

set(PYLIBCZI_PYBIND11 _aicspylibczi/pb_bindings.cpp _aicspylibczi/pb_helpers.h _aicspylibczi/pb_helpers.cpp _aicspylibczi/pb_caster_ImagesContainer.h
        _aicspylibczi/pb_caster_BytesIO.h _aicspylibczi/pb_caster_libCZI_DimensionIndex.h _aicspylibczi/CSimpleStreamImplFromFd.h
        _aicspylibczi/CSimpleStreamImplFromFd.cpp _aicspylibczi/pb_caster_DimIndex.h)

set(TARGET_ONE libczi_c++_extension)
set(TARGET_TWO _aicspylibczi)
set(TARGET_THREE profile_me)

add_library(${TARGET_ONE} STATIC ${PYLIBCZI_C_SRC} ${PYLIBCZI_C_SRC_HEADERS})
target_include_directories(${TARGET_ONE} PUBLIC ${CMAKE_SOURCE_DIR}/libCZI/Src)
target_link_libraries(${TARGET_ONE} INTERFACE libCZIStatic JxrDecodeStatic)
add_dependencies(${TARGET_ONE} libCZIStatic JxrDecodeStatic)

pybind11_add_module(${TARGET_TWO} MODULE ${PYLIBCZI_C_SRC} ${PYLIBCZI_PYBIND11})
target_include_directories(${TARGET_TWO} PUBLIC ${CMAKE_SOURCE_DIR}/libCZI/Src)
target_link_libraries(${TARGET_TWO} PRIVATE libCZIStatic JxrDecodeStatic)
add_dependencies(${TARGET_TWO} libCZIStatic JxrDecodeStatic)
add_custom_command(TARGET ${TARGET_TWO} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_TWO}> ${PROJECT_SOURCE_DIR}
        )

add_subdirectory(c_tests)

add_executable(${TARGET_THREE} main.cpp)
target_link_libraries(${TARGET_THREE} ${TARGET_ONE})

if(DEFINED ENV{BUILD_DOCS})
    add_subdirectory(docs)
endif()
