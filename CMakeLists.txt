cmake_minimum_required(VERSION 3.0)

if (WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif ()

add_subdirectory(NatsuLib)
