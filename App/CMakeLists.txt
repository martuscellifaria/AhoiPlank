cmake_minimum_required(VERSION 3.28)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    @ONLY
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Gui)
find_package(LayerShellQt REQUIRED)
qt_standard_project_setup()

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()

qt_add_executable(${PROJECT_NAME} src/${PROJECT_NAME}.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE PROJECT_QML_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml
)

foreach(f IN LISTS PROJECT_QML_FILES)
    file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}/qml" "${f}")
    set_source_files_properties("${f}" PROPERTIES
        QT_RESOURCE_ALIAS "${_rel}")
endforeach()

qt_add_qml_module(
    ${PROJECT_NAME}
    URI ${PROJECT_NAME}Qml
    VERSION 1.0
    QML_FILES ${PROJECT_QML_FILES}
)

include(AhoiCppSubdirs.cmake)

add_library(ahoicpp_externals_interface INTERFACE)
target_link_libraries(ahoicpp_externals_interface INTERFACE ahoicpp_externals)
target_link_libraries(ahoicpp_externals_interface INTERFACE ${AHOICPP_EXTERNALS_TARGETS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ahoicpp_externals_interface
    Qt6::Quick
    Qt6::Gui
    LayerShellQt::Interface
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in
        ${CMAKE_CURRENT_SOURCE_DIR}/version.rc
        @ONLY
    )
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.rc)
else()
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/version.c.in
        ${CMAKE_CURRENT_SOURCE_DIR}/version.c
        @ONLY
    )
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/version.c)
endif()
