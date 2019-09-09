
set(UNIT_TEST_SOURCE_LIST test_main.cpp test_Reader.cpp)
set(TARGET_NAME test_libczi_c++_extension)

add_executable(${TARGET_NAME} ${UNIT_TEST_SOURCE_LIST})
target_link_libraries(${TARGET_NAME} PUBLIC libczi_c++_extension libCZI)
add_dependencies(${TARGET_NAME} libczi_c++_extension libCZI)
