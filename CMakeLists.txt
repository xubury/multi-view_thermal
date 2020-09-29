cmake_minimum_required(VERSION 3.9)
project(multi-view_thermal VERSION 0.1.0)

include(ExternalProject)

find_package(fmt REQUIRED)
find_package(wxWidgets REQUIRED net core base gl ribbon adv aui)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(TIFF REQUIRED)

find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glbinding REQUIRED)
find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -Wundef -pedantic -DEIGEN_DONT_VECTORIZE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse -funroll-loops")
endif()

add_subdirectory(elibs)

add_executable(multi_view src/MainApp.cpp include/MainApp.hpp
                          src/MainFrame.cpp include/MainFrame.hpp
                          include/Image.hpp src/Image.cpp
                          src/GLPanel.cpp include/GLPanel.hpp
                          src/Shader.cpp include/Shader.hpp
                          src/RenderTarget.cpp include/RenderTarget.hpp
                          src/Axis.cpp include/Axis.hpp
                          src/Camera.cpp include/Camera.hpp include/Frustum.cpp include/Frustum.hpp)

add_dependencies(multi_view ext_mve)

target_compile_definitions(multi_view PRIVATE ${wxWidgets_DEFINITIONS})
target_include_directories(multi_view PRIVATE include ${wxWidgets_INCLUDE_DIRS} elibs/mve/libs)
target_precompile_headers(multi_view PRIVATE include/pch.hpp)

target_link_directories(multi_view PRIVATE elibs/mve/libs/mve elibs/mve/libs/util elibs/mve/libs/sfm)
target_link_libraries(multi_view ${wxWidgets_LIBRARIES} fmt::fmt
                      OpenGL::GL glbinding::glbinding glm
                      mve_sfm mve mve_util  ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES})

add_custom_target(shader_files
                  COMMAND ${CMAKE_COMMAND} -E copy_directory
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader
                  ${CMAKE_BINARY_DIR}
                  COMMENT "copying shader files...")
add_dependencies(multi_view shader_files)