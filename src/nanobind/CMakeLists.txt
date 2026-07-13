
if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

find_package(Python 3.9 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/external/nanobind ${CMAKE_SOURCE_DIR}/external/nanobind/build)
include_directories(${CMAKE_SOURCE_DIR}/include)

nanobind_add_module(humma humma.cpp)
target_link_libraries(humma PRIVATE zaphod)