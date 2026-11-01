find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(ThemeConfig STATIC src/ThemeConfig.cpp include/ThemeConfig/ThemeConfig.h)
set_target_properties(ThemeConfig PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(ThemeConfig ahoicpp_externals Qt6::Core Qt6::Gui)
target_compile_features(ThemeConfig PUBLIC cxx_std_23)
target_include_directories(ThemeConfig PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ThemeConfig)
