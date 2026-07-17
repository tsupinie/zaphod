
include(CTest)

get_target_property(G2C_INCLUDE g2c::g2c INTERFACE_INCLUDE_DIRECTORIES)

file(GLOB test_files "*.cpp")

foreach(file ${test_files})
    get_filename_component(test_name ${file} NAME_WE)

    add_executable(${test_name} ${file})
    target_compile_options(${test_name} PRIVATE -std=c++17 -O3)
    target_include_directories(${test_name} PRIVATE src ${G2C_INCLUDE})
    target_link_libraries(${test_name} PRIVATE zaphod)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()