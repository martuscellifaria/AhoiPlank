find_package(Qt6 REQUIRED COMPONENTS Core Quick)

add_library(IconProvider STATIC src/IconProvider.cpp include/IconProvider/IconProvider.h)
set_target_properties(IconProvider PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(IconProvider ahoicpp_externals Qt6::Core Qt6::Quick)
target_compile_features(IconProvider PUBLIC cxx_std_23)
target_include_directories(IconProvider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/IconProvider)
add_subdirectory(test)
