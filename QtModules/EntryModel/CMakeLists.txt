find_package(Qt6 REQUIRED COMPONENTS Core Quick)

add_library(EntryModel STATIC
    src/EntryModel.cpp
    src/DesktopSource.cpp
    src/BookmarksSource.cpp
    src/DotfilesSource.cpp
    src/SshSource.cpp
    src/DesktopLauncher.cpp
    src/BookmarkLauncher.cpp
    src/DotfileLauncher.cpp
    src/SshLauncher.cpp
    include/EntryModel/EntryModel.h
    include/EntryModel/Entry.h
    include/EntryModel/EntrySource.h
    include/EntryModel/EntryLauncher.h
    include/EntryModel/DesktopSource.h
    include/EntryModel/BookmarksSource.h
    include/EntryModel/DotfilesSource.h
    include/EntryModel/SshSource.h
    include/EntryModel/DesktopLauncher.h
    include/EntryModel/BookmarkLauncher.h
    include/EntryModel/DotfileLauncher.h
    include/EntryModel/SshLauncher.h
)
set_target_properties(EntryModel PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_link_libraries(EntryModel ahoicpp_externals Qt6::Core Qt6::Quick)
target_compile_features(EntryModel PUBLIC cxx_std_23)
target_include_directories(EntryModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/EntryModel)
