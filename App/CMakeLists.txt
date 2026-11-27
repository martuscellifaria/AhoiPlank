configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Gui)
find_package(LayerShellQt REQUIRED)
qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(${PROJECT_NAME} src/AhoiPlank.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(PROJECT_QML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/Main.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/StatusBar.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/ErrorToast.qml
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

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-Wl,--gc-sections>
    )
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.c.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.c
    @ONLY
)
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.c)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_add_resources(${PROJECT_NAME} "app_icons"
    PREFIX "/icons"
    BASE "resources"
    FILES resources/plank.png
)

install(FILES resources/plank.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
    RENAME ahoiplank.png
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ahoiplank.desktop)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ahoiplank.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
endif()
