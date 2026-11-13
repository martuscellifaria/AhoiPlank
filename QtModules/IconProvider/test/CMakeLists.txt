set(TEST_TARGET IconProviderTest)

file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${TEST_TARGET} ${TEST_SOURCES})
target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/externals/googletest/googletest/include)

target_compile_features(${TEST_TARGET} PUBLIC cxx_std_23)
target_link_libraries(${TEST_TARGET} PRIVATE
    # You can link everything you may need here
    IconProvider
    gtest
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})
