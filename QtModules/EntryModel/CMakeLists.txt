find_package(Qt6 REQUIRED COMPONENTS Core Quick)

add_library(EntryModel STATIC src/EntryModel.cpp include/EntryModel/EntryModel.h)
set_target_properties(EntryModel PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(EntryModel ahoicpp_externals Qt6::Core Qt6::Quick)
target_compile_features(EntryModel PUBLIC cxx_std_23)
target_include_directories(EntryModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/EntryModel)
