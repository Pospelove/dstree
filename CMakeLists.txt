cmake_minimum_required(VERSION 3.0)
project(dstree)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS OFF)

add_library(dstree
  .clang-format
  dstree/src/array.cpp
  dstree/src/tree.cpp
  dstree/src/array.hpp
  dstree/src/tree.hpp
  dstree/src/dstree.cpp
  dstree/include/dstree/dstree.hpp
)
target_include_directories(dstree PUBLIC dstree/include)

add_executable(console_app .clang-format console_app/main.cpp)
target_link_libraries(console_app PRIVATE dstree)

if (BUILD_TESTS)
  set(PMM_REVISION b53a73c24bf4bb02f1081560ec83e50cecc4c6e0)
  set(VCPKG_REVISION f1bef4aa7ca7e2a6ea4f5dfe4850d95fce60b431)

  file(DOWNLOAD
    https://raw.githubusercontent.com/vector-of-bool/pmm/${PMM_REVISION}/pmm.cmake
    ${CMAKE_BINARY_DIR}/pmm.cmake
  )
  include(${CMAKE_BINARY_DIR}/pmm.cmake)

  pmm(VCPKG REVISION ${VCPKG_REVISION} REQUIRES catch2)
  find_package(Catch2 CONFIG REQUIRED)

  add_executable(tests
    .clang-format
    tests/main.cpp
    tests/array_test.cpp
    tests/tree_test.cpp
  )
  target_link_libraries(tests PRIVATE dstree Catch2::Catch2)
  target_include_directories(tests PRIVATE tests dstree/src)
endif()
