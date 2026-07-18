find_package(Qt6 6.8 REQUIRED COMPONENTS Core Core5Compat Test Widgets)

qt_add_executable(
    serialtest_util_tests
    test_util.cpp
    ../src/metadata.cpp
    ../src/metadata.h
    ../src/sessiondata.cpp
    ../src/sessiondata.h
    ../src/util.cpp
    ../src/util.h
)

target_include_directories(
    serialtest_util_tests
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src"
)
target_link_libraries(
    serialtest_util_tests
    PRIVATE
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME serialtest_util_tests COMMAND serialtest_util_tests)
set_tests_properties(
    serialtest_util_tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
