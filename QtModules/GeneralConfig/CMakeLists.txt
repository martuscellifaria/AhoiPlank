find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(GeneralConfig STATIC src/GeneralConfig.cpp include/GeneralConfig/GeneralConfig.h)
set_target_properties(GeneralConfig PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(GeneralConfig ahoicpp_externals Qt6::Core Qt6::Gui)
target_compile_features(GeneralConfig PUBLIC cxx_std_23)
target_include_directories(GeneralConfig PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/GeneralConfig)
