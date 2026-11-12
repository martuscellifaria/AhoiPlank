find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(Test_EntryModel
    Test_EntryModel.cpp
)

set_target_properties(Test_EntryModel PROPERTIES AUTOMOC ON)

target_link_libraries(Test_EntryModel PRIVATE
    EntryModel
    Qt6::Test
)

add_test(NAME Test_EntryModel COMMAND Test_EntryModel)
