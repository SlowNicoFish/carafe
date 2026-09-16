qt_add_executable(carafe_storage_tests
    storage_test.cpp
    ../src/game.cpp
    ../src/storage.cpp
)

target_include_directories(carafe_storage_tests PRIVATE ../src)
target_link_libraries(carafe_storage_tests PRIVATE Qt6::Core Qt6::Test)

add_test(NAME carafe_storage_tests COMMAND carafe_storage_tests)