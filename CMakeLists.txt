cmake_minimum_required(VERSION 3.15...3.27)
project(zaphod)

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(g2c)

find_package(PROJ CONFIG QUIET)
if(NOT PROJ_FOUND)
    message(STATUS "PROJ CMake config not found, attempting PkgConfig...")
    pkg_check_modules(PROJ REQUIRED IMPORTED_TARGET proj)
    if(NOT TARGET PROJ::proj)
        add_library(PROJ::proj ALIAS PkgConfig::PROJ)
    endif()
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(
    GLOB SOURCES CONFIGURE_DEPENDS 
    "src/zaphod/*.cpp"
)

include_directories(include)

add_library(zaphod ${SOURCES})

target_compile_options(zaphod PRIVATE -std=c++17 -O3)
target_link_libraries(zaphod PRIVATE g2c::g2c PROJ::proj) # stdc++fs

if (NOT SKBUILD)
    install(TARGETS zaphod LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY include/zaphod INCLUDE DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    add_subdirectory(tests)
endif()