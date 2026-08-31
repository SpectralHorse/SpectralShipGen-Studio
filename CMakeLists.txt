cmake_minimum_required(VERSION 3.16)
project(PixelShipGeneratorGui LANGUAGES CXX)

if(MSVC)
    add_compile_options(/wd26812)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

option(PIXEL_SHIP_GENERATOR_FETCH_SFML "Fetch SFML 2.6.x for application targets" ON)
if(PIXEL_SHIP_GENERATOR_FETCH_SFML)
    include(FetchContent)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x)
    FetchContent_MakeAvailable(SFML)
endif()

set(PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR "" CACHE PATH "Path to a PixelShipGenerator Library source checkout for local GUI development")
if(NOT TARGET PixelShipGeneratorCore)
    if(NOT PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR)
        message(FATAL_ERROR "Set PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR to the PixelShipGenerator Library checkout.")
    endif()
    if(NOT EXISTS "${PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR does not contain a Library CMakeLists.txt: ${PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR}")
    endif()
    set(PIXEL_SHIP_GENERATOR_BUILD_DIAGNOSTICS_CLI OFF CACHE BOOL "" FORCE)
    set(PIXEL_SHIP_GENERATOR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PIXEL_SHIP_GENERATOR_BUILD_CORE_REGRESSION OFF CACHE BOOL "" FORCE)
    add_subdirectory("${PIXEL_SHIP_GENERATOR_LIBRARY_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/PixelShipGeneratorLibrary")
endif()

include(CTest)
include(cmake/PixelShipGeneratorGuiTargets.cmake)
