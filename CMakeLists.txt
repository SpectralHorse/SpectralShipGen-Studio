cmake_minimum_required(VERSION 3.16)
project(SpectralShipGenStudio LANGUAGES CXX)

if(MSVC)
    add_compile_options(/wd26812)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

option(SPECTRAL_SHIP_GEN_FETCH_SFML "Fetch SFML 2.6.x for application targets" ON)
if(SPECTRAL_SHIP_GEN_FETCH_SFML)
    include(FetchContent)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.x)
    FetchContent_MakeAvailable(SFML)
endif()

set(SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR "" CACHE PATH "Path to a SpectralShipGen Library source checkout for local Studio development")
option(SPECTRAL_SHIP_GEN_FETCH_LIBRARY "Fetch SpectralShipGen when no local Library checkout is supplied" OFF)

if(NOT TARGET SpectralShipGenCore)
    set(SPECTRAL_SHIP_GEN_BUILD_DIAGNOSTICS_CLI OFF CACHE BOOL "" FORCE)
    set(SPECTRAL_SHIP_GEN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SPECTRAL_SHIP_GEN_BUILD_CORE_REGRESSION OFF CACHE BOOL "" FORCE)

    if(SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR)
        if(NOT EXISTS "${SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR does not contain a Library CMakeLists.txt: ${SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR}")
        endif()
        add_subdirectory("${SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/SpectralShipGenLibrary")
    elseif(SPECTRAL_SHIP_GEN_FETCH_LIBRARY)
        include(FetchContent)
        FetchContent_Declare(SpectralShipGen
            GIT_REPOSITORY https://github.com/SpectralHorse/SpectralShipGen.git
            GIT_TAG master)
        FetchContent_MakeAvailable(SpectralShipGen)
    else()
        message(FATAL_ERROR
            "SpectralShipGen Studio requires the SpectralShipGen Library. "
            "Set SPECTRAL_SHIP_GEN_LIBRARY_SOURCE_DIR to a local checkout or enable SPECTRAL_SHIP_GEN_FETCH_LIBRARY.")
    endif()
endif()

include(CTest)
include(cmake/SpectralShipGenStudioTargets.cmake)
