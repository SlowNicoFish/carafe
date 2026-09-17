qt_add_executable(carafe_storage_tests
    storage_test.cpp
    ../src/game.cpp
    ../src/storage.cpp
)

target_include_directories(carafe_storage_tests PRIVATE ../src)
target_link_libraries(carafe_storage_tests PRIVATE Qt6::Core Qt6::Test)

add_test(NAME carafe_storage_tests COMMAND carafe_storage_tests)

qt_add_executable(carafe_gamemodel_tests
    gamemodel_test.cpp
    ../src/game.cpp
    ../src/gamemodel.cpp
)

target_include_directories(carafe_gamemodel_tests PRIVATE ../src)
target_link_libraries(carafe_gamemodel_tests PRIVATE Qt6::Core Qt6::Test)

add_test(NAME carafe_gamemodel_tests COMMAND carafe_gamemodel_tests)

qt_add_executable(carafe_gamelibrary_tests
    gamelibrary_test.cpp
    ../src/game.cpp
    ../src/gamemodel.cpp
    ../src/gamelibrary.cpp
    ../src/icoextract.cpp
    ../src/storage.cpp
)

target_include_directories(carafe_gamelibrary_tests PRIVATE ../src)
target_link_libraries(carafe_gamelibrary_tests PRIVATE Qt6::Core Qt6::Concurrent Qt6::Test)

add_test(NAME carafe_gamelibrary_tests COMMAND carafe_gamelibrary_tests)