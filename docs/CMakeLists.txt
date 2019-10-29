cmake_minimum_required(VERSION 2.8.12)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


find_package(Sphinx)

if(NOT DEFINED SPHINX_THEME)
    set(SPHINX_THEME "sphinx_rtd_theme")

endif()

if(NOT DEFINED SPHINX_THEME_DIR)
    set(SPHINX_THEME_DIR)

endif()

project(PyDocs)

# set(CMAKE_VERBOSE_MAKEFILE ON)

# configured documentation tools and intermediate build results
set(BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")

# Sphinx cache with pickled ReST documents
set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")

# HTML output directory
set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
    "${BINARY_BUILD_DIR}/conf.py"
    @ONLY
    )

add_custom_target(docs ALL ${SPHINX_EXE}
        -q -b html
        -c "${BINARY_BUILD_DIR}"
        -d "${SPHINX_CACHE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SPHINX_HTML_DIR}"
    COMMENT "Building HTML documentation with Sphinx"
    )

set(IMAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/html/images")

make_directory(${IMAGE_DIR})

set(IMAGES ${CMAKE_SOURCE_DIR}/images/colony.png ${CMAKE_SOURCE_DIR}/images/histo.png)

add_custom_command(TARGET docs POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${IMAGES} ${IMAGE_DIR}
        )

file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/html/.nojekyll)
