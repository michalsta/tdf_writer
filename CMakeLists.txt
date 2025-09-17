cmake_minimum_required(VERSION 3.15)


project(wnet LANGUAGES CXX)
find_package(Python 3.8
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule)
find_package(nanobind REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 20)

nanobind_add_module(tdf_writer_cpp
    NB_STATIC NOMINSIZE
    src/tdf_writer/cpp/tdf_writer/dispatcher.cpp)

install(TARGETS tdf_writer_cpp LIBRARY DESTINATION tdf_writer)

# Uncomment the following line to enable detailed debug prints in the C++ code
# target_compile_definitions(tdf_writer PRIVATE DO_TONS_OF_PRINTS)
