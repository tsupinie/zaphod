cmake_minimum_required(VERSION 3.15...3.27)
project(grib2spc)

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

# set(Python_ROOT_DIR /users/tsupinie/software/miniconda3/envs/grefs2)

# find_package(Python 3.8 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)
find_library(G2C g2c)
get_filename_component(G2CLIB ${G2C} DIRECTORY)
get_filename_component(G2CROOT ${G2CLIB} DIRECTORY)
include_directories(${G2CROOT}/include)

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

target_compile_options(grib2spc PRIVATE -std=c++17)

target_link_libraries(grib2spc PRIVATE ${G2C} stdc++fs)

install(TARGETS grib2spc DESTINATION lib)