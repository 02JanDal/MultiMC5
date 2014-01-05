# run the full test with `make full_test`

find_package(Qt5 COMPONENTS Test Core Network Widgets)

include_directories(${MMC_SRC})

set(FULL_TEST_SOURCES
 TesterInterface.h
 Tester.h
 Tester.cpp
 Executer.h
 Executer.cpp
)

add_library(FullTest SHARED ${FULL_TEST_SOURCES})
qt5_use_modules(FullTest Test Core Network Widgets)
add_dependencies(MultiMC FullTest)
