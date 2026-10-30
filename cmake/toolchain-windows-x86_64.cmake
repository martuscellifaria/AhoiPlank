set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(XWIN_ROOT "/opt/xwin")
set(LLVM_ROOT "/usr/lib/llvm-21")
set(TARGET_TRIPLE "x86_64-pc-windows-msvc")

set(XWIN_CRT_INCLUDE "${XWIN_ROOT}/crt/include")
set(XWIN_CRT_LIB "${XWIN_ROOT}/crt/lib/x86_64")

set(XWIN_SDK_INCLUDE "${XWIN_ROOT}/sdk/include")
set(XWIN_SDK_LIB "${XWIN_ROOT}/sdk/lib")

set(XWIN_SDK_UCRT_INCLUDE "${XWIN_SDK_INCLUDE}/ucrt")
set(XWIN_SDK_SHARED_INCLUDE "${XWIN_SDK_INCLUDE}/shared")
set(XWIN_SDK_UM_INCLUDE "${XWIN_SDK_INCLUDE}/um")
set(XWIN_SDK_WINRT_INCLUDE "${XWIN_SDK_INCLUDE}/winrt")
set(XWIN_SDK_CPPWINRT_INCLUDE "${XWIN_SDK_INCLUDE}/cppwinrt")

set(XWIN_SDK_UCRT_LIB "${XWIN_SDK_LIB}/ucrt/x86_64")
set(XWIN_SDK_UM_LIB "${XWIN_SDK_LIB}/um/x86_64")

if(NOT EXISTS "${XWIN_CRT_INCLUDE}/vector")
    message(FATAL_ERROR "xwin C++ headers not found: ${XWIN_CRT_INCLUDE}")
endif()

if(NOT EXISTS "${XWIN_SDK_UM_INCLUDE}/windows.h")
    message(FATAL_ERROR "Windows SDK headers not found: ${XWIN_SDK_UM_INCLUDE}")
endif()

if(NOT EXISTS "${XWIN_CRT_LIB}/libcmt.lib")
    message(FATAL_ERROR "xwin CRT libraries not found: ${XWIN_CRT_LIB}")
endif()

if(NOT EXISTS "${XWIN_CRT_LIB}/oldnames.lib")
    message(FATAL_ERROR "xwin oldnames.lib not found: ${XWIN_CRT_LIB}")
endif()

if(NOT EXISTS "${LLVM_ROOT}/bin/clang-cl")
    message(FATAL_ERROR "clang-cl not found: ${LLVM_ROOT}/bin/clang-cl")
endif()

if(NOT EXISTS "${LLVM_ROOT}/bin/lld-link")
    message(FATAL_ERROR "lld-link not found: ${LLVM_ROOT}/bin/lld-link")
endif()

set(CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang-cl")
set(CMAKE_CXX_COMPILER "${LLVM_ROOT}/bin/clang-cl")
set(CMAKE_LINKER "${LLVM_ROOT}/bin/lld-link")

set(CMAKE_C_COMPILER_TARGET "${TARGET_TRIPLE}")
set(CMAKE_CXX_COMPILER_TARGET "${TARGET_TRIPLE}")

set(CMAKE_RC_COMPILER "${LLVM_ROOT}/bin/llvm-rc")
set(CMAKE_AR "${LLVM_ROOT}/bin/llvm-lib")
set(CMAKE_C_COMPILER_AR "${LLVM_ROOT}/bin/llvm-lib")
set(CMAKE_CXX_COMPILER_AR "${LLVM_ROOT}/bin/llvm-lib")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

set(CMAKE_C_FLAGS_INIT "/imsvc ${XWIN_CRT_INCLUDE} /imsvc ${XWIN_SDK_UCRT_INCLUDE} /imsvc ${XWIN_SDK_SHARED_INCLUDE} /imsvc ${XWIN_SDK_UM_INCLUDE} /imsvc ${XWIN_SDK_WINRT_INCLUDE} /imsvc ${XWIN_SDK_CPPWINRT_INCLUDE}")

set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -clang:-Wno-error=character-conversion")
set(CMAKE_RC_FLAGS_INIT "-I${XWIN_CRT_INCLUDE} -I${XWIN_SDK_UCRT_INCLUDE} -I${XWIN_SDK_SHARED_INCLUDE} -I${XWIN_SDK_UM_INCLUDE} -I${XWIN_SDK_WINRT_INCLUDE} -I${XWIN_SDK_CPPWINRT_INCLUDE}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld-link /libpath:${XWIN_CRT_LIB} /libpath:${XWIN_SDK_UCRT_LIB} /libpath:${XWIN_SDK_UM_LIB}")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
