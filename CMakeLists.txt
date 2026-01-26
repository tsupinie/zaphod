cmake_minimum_required(VERSION 3.15...3.27)
project(grib2spc)

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

# set(Python_ROOT_DIR /users/tsupinie/software/miniconda3/envs/grefs2)

# find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)
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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(
    GLOB SOURCES CONFIGURE_DEPENDS 
    "src/*.cpp"
)

add_library(grib2spc ${SOURCES})

target_compile_options(grib2spc PRIVATE -std=c++17 -O3)
target_link_libraries(grib2spc PRIVATE g2c::g2c stdc++fs)

install(TARGETS grib2spc DESTINATION lib)

include(CTest)

add_executable(test_projection tests/test_projection.cpp)
target_compile_options(test_projection PRIVATE -std=c++17 -O3)
target_include_directories(test_projection PRIVATE src)
target_link_libraries(test_projection PRIVATE g2c::g2c PROJ::proj)
add_test(NAME test_projection COMMAND test_projection)

add_executable(test_templates tests/test_templates.cpp)
target_compile_options(test_templates PRIVATE -std=c++17)
target_include_directories(test_templates PRIVATE src)
target_link_libraries(test_templates PRIVATE g2c::g2c)
add_test(NAME test_templates COMMAND test_templates)