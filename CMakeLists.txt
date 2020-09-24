cmake_minimum_required(VERSION 3.9)
project(multi-view_thermal VERSION 0.1.0)

include(ExternalProject)

find_package(fmt REQUIRED)
find_package(wxWidgets REQUIRED net core base gl ribbon adv aui)

find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glbinding REQUIRED)

add_subdirectory(elibs)

add_executable(multi_view src/MainApp.cpp include/MainApp.hpp src/MainFrame.cpp include/MainFrame.hpp)

add_dependencies(multi_view ext_mve)

target_compile_options(multi_view PRIVATE -Wall -Winvalid-pch -Wunused-variable)
target_compile_definitions(multi_view PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(multi_view PRIVATE include ${wxWidgets_INCLUDE_DIRS} elibs/mve/libs)
target_precompile_headers(multi_view PRIVATE include/pch.hpp)

target_link_directories(multi_view PRIVATE elibs/mve/libs/mve elibs/mve/libs/util)
target_link_libraries(multi_view ${wxWidgets_LIBRARIES} fmt::fmt OpenGL::GL glbinding::glbinding glm
                      mve mve_util)
