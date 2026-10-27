find_package(Qt6 REQUIRED COMPONENTS Core Quick)

add_library(entry_model STATIC src/entry_model.cpp include/entry_model/entry_model.h)
set_target_properties(entry_model PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(entry_model ahoicpp_externals Qt6::Core Qt6::Quick)
target_compile_features(entry_model PUBLIC cxx_std_23)
target_include_directories(entry_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/entry_model)
