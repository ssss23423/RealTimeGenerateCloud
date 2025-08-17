cmake_minimum_required(VERSION 3.22)

# The options need to be the same as Open3D's default
# If Open3D is configured and built with custom options, you'll also need to
# specify the same custom options.
option(STATIC_WINDOWS_RUNTIME "Use static (MT/MTd) Windows runtime" OFF)
if(STATIC_WINDOWS_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

project(GenerateCloudCore VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

if(WIN32)
    set(OpenCV_DIR "D:/Software/OpenCV/OpenCV-VC17-Build-OpenCV-4.10.0-x64")
    set(GLFW3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/glfw/GLFW")
    set(GLFW3_LIBRARY "${CMAKE_SOURCE_DIR}/third_party/glfw/lib-vc2022/glfw3.lib")
    file(TO_CMAKE_PATH "D:/Software/MVTec/HALCON-24.11-Progress-Steady/examples" HALCONEXAMPLES)

    file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/third_party/open3d-devel-windows-amd64-0.19.0" Open3DROOT)
    find_package(Open3D HINTS ${Open3DROOT}/CMake)
else()
    file(TO_CMAKE_PATH "$ENV{HALCONROOT}/examples" HALCONEXAMPLES)
    find_package(glfw3 REQUIRED)
endif()

find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${HALCONEXAMPLES}/cmake")
include(UseHALCON)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
)

file(GLOB ORIGIN
    ${CMAKE_SOURCE_DIR}/*.h
    ${CMAKE_SOURCE_DIR}/*.cpp
)

file(GLOB_RECURSE HERDERS
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/third_party/*.h
    
)

file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/third_party/*.cpp
)

set(PROJECT_SOURCES
    ${ORIGIN}
    ${HERDERS}
    ${SOURCES}
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/cli
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/gui
    ${CMAKE_SOURCE_DIR}/include/messages
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/cmdline
    ${CMAKE_SOURCE_DIR}/third_party/nlohmann
    ${CMAKE_SOURCE_DIR}/third_party/imgui/
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_SOURCE_DIR}/third_party/glfw/GLFW
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW3_LIBRARY})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${PCL_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
target_link_libraries(${PROJECT_NAME} PRIVATE HALCON::CppInt)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE Open3D::Open3D)

# On Windows, when BUILD_SHARED_LIBS, copy .dll to the executable directory
if(WIN32)
    get_target_property(open3d_type Open3D::Open3D TYPE)
    if(open3d_type STREQUAL "SHARED_LIBRARY")
        message(STATUS "Will copy Open3D.dll to ${EXECUTABLE_OUTPUT_PATH}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy
                                ${Open3DROOT}/bin/Open3D.dll
                                ${EXECUTABLE_OUTPUT_PATH}/$<CONFIG>)
    endif()
endif()