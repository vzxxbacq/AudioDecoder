enable_testing()
add_executable(test_decode decode_test.cpp)

set(GTEST_LIB /usr/local/lib/libgtest.so)
target_link_libraries(test_decode ${GTEST_LIB} decode)
add_test(test_decode test_decode)
