cmake_minimum_required(VERSION 2.8)
PROJECT(SPEC_ANALYSER)

INCLUDE ( ./cmake/FindLIBUSB.cmake )
INCLUDE ( ./cmake/FindFLTK.cmake )

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(X11 REQUIRED)

FIND_PACKAGE(OpenMP)

if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

IF(NOT LIBUSB_1_FOUND)
MESSAGE (FATAL_ERROR "LibUSB-1 Not found.")
ENDIF()

IF(NOT LIBUSB_1_FOUND)
MESSAGE (FATAL_ERROR "LibUSB-1 Not found.")
ENDIF()

ADD_SUBDIRECTORY(lib-sdr)
ADD_SUBDIRECTORY(src)

