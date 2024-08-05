cmake_minimum_required(VERSION 3.14)

project(midll_test LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "D:/program/Qt/6.7.2/mingw_64")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)

add_executable(midll_test
  main.cpp
)
target_link_libraries(midll_test Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(midll_test Qt6::Core Qt6::WebSockets)

include(GNUInstallDirs)
install(TARGETS midll_test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
