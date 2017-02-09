cmake_minimum_required(VERSION 3.0)
project(zlib C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wno-implicit-function-declaration")

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SOURCE_FILES
    include/crc32.h
    include/deflate.h
    include/gzguts.h
    include/inffast.h
    include/inffixed.h
    include/inflate.h
    include/inftrees.h
    include/trees.h
    include/zconf.h
    include/zlib.h
    include/zutil.h
    src/adler32.c
    src/compress.c
    src/crc32.c
    src/deflate.c
    src/gzclose.c
    src/gzlib.c
    src/gzread.c
    src/gzwrite.c
    src/infback.c
    src/inffast.c
    src/inflate.c
    src/inftrees.c
    src/trees.c
    src/uncompr.c
    src/zutil.c)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME})

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
