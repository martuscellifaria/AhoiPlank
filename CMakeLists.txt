cmake_minimum_required(VERSION 3.28)

if(DEFINED VERSION_ARG)
    set(PROJECT_VERSION ${VERSION_ARG})
else()
    set(PROJECT_VERSION "99.99.99") 
endif()
if(DEFINED COMPANY_ARG)
	set(PROJECT_COMPANY ${COMPANY_ARG})
else()
	set(PROJECT_COMPANY "AhoiTools") 
endif()
if(DEFINED DESCRIPTION_ARG)
	set(PROJECT_DESCRIPTION ${DESCRIPTION_ARG})
else()
	set(PROJECT_DESCRIPTION "Your project is owned by AhoiTools") 
endif()

project(AhoiPlank VERSION ${PROJECT_VERSION})
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(VER_MAJOR ${PROJECT_VERSION_MAJOR})
set(VER_MINOR ${PROJECT_VERSION_MINOR})
set(VER_PATCH ${PROJECT_VERSION_PATCH})
set(VERSION ${PROJECT_VERSION})
set(COMPANY ${PROJECT_COMPANY})
set(DESCRIPTION ${PROJECT_DESCRIPTION})
set(PRODUCT ${PROJECT_NAME})
string(TIMESTAMP COPYRIGHT_YEAR "%Y")
set(COPYRIGHT "Copyright ${COPYRIGHT_YEAR}")

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
string(TIMESTAMP BUILD_DATE "%Y%m%d")

find_package(Git QUIET)
set(GIT_HASH "unknown")
set(GIT_HASH_SHORT "unknown")
set(GIT_BRANCH "unknown")
set(GIT_DIRTY "")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT GIT_STATUS STREQUAL "")
        set(GIT_DIRTY "-dirty")
    endif()
endif()

include(AhoiCppProject.cmake)
include(AhoiCppExternals.cmake)
add_library(ahoicpp_externals INTERFACE)

foreach(PATH ${AHOICPP_EXTERNALS_INCLUDE_PATHS})
    file(GLOB EXPANDED_PATHS ${PATH})
    foreach(EXPANDED ${EXPANDED_PATHS})
        if(IS_DIRECTORY ${EXPANDED})
            target_include_directories(ahoicpp_externals INTERFACE ${EXPANDED})
        endif()
    endforeach()
    if(NOT PATH MATCHES "\\*")
        if(EXISTS ${PATH})
            target_include_directories(ahoicpp_externals INTERFACE ${PATH})
        endif()
    endif()
endforeach()
add_subdirectory(App)

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(Tests)
endif()
