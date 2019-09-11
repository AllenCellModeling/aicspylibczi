
set(UNIT_TEST_SOURCE_LIST test_main.cpp test_Reader.cpp test_Iterator.cpp test_Image.cpp)
set(TARGET_NAME test_libczi_c++_extension)
add_compile_definitions(_STATICLIBBUILD)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(../_pylibczi)
add_executable(${TARGET_NAME} ${UNIT_TEST_SOURCE_LIST})
target_link_libraries(${TARGET_NAME} PRIVATE libczi_c++_extension libCZIStatic JxrDecodeStatic)
add_dependencies(${TARGET_NAME} libczi_c++_extension libCZIStatic JxrDecodeStatic)

add_custom_target(build-time-make-directory ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>/resources)
add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/c_tests/resources $<TARGET_FILE_DIR:${TARGET_NAME}>/resources)
