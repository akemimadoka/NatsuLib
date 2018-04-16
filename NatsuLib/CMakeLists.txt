cmake_minimum_required(VERSION 3.0)
project(NatsuLib CXX)

message(STATUS "Operation system is ${CMAKE_SYSTEM}")

set(CMAKE_CXX_STANDARD 17)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -Wno-unused")
endif()

include_directories(${zlib_INCLUDE_DIRS})

set(SOURCE_FILES
    natBinary.cpp
    natBinary.h
    natCompression.cpp
    natCompression.h
    natCompressionStream.cpp
    natCompressionStream.h
    natConcepts.h
    natConfig.h
    natConsole.cpp
    natConsole.h
    natContainer.h
    natCryptography.cpp
    natCryptography.h
    natDelegate.h
    natEnvironment.cpp
    natEnvironment.h
    natEvent.cpp
    natEvent.h
    natException.cpp
    natException.h
    natInfixOperator.h
    natInterface.cpp
    natInterface.h
    natLinq.h
    natLocalFileScheme.cpp
    natLocalFileScheme.h
    natLog.cpp
    natLog.h
    natMat.h
    natMath.h
    natMisc.cpp
    natMisc.h
    natMultiThread.cpp
    natMultiThread.h
    natNamedPipe.cpp
    natNamedPipe.h
    natNode.h
    natProperty.h
    natQuat.h
    natRefObj.h
    natRelationalOperator.h
    natStackWalker.cpp
    natStackWalker.h
    natStopWatch.cpp
    natStopWatch.h
    natStream.cpp
    natStream.h
    natStreamHelper.h
    natString.cpp
    natString.h
    natStringUtil.h
    natTask.cpp
    natTask.h
    natText.h
    natTransform.h
    natType.h
    natUtil.cpp
    natUtil.h
    natVec.h
    natVFS.cpp
    natVFS.h
    stdafx.cpp
    stdafx.h
    targetver.h)

include_directories(${zlib_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} zlib)
else()
    target_link_libraries(${PROJECT_NAME} zlib pthread)
endif()
