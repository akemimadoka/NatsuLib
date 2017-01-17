cmake_minimum_required(VERSION 3.0)
project(NatsuLib)

message(STATUS "Operation system is ${CMAKE_SYSTEM}")

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wno-missing-field-initializers")
endif()

set(SOURCE_FILES
    NatsuLib/natConcepts.h
    NatsuLib/natConfig.h
    NatsuLib/natConsole.cpp
    NatsuLib/natConsole.h
    NatsuLib/natDelegate.h
    NatsuLib/natEnvironment.cpp
    NatsuLib/natEnvironment.h
    NatsuLib/natEvent.cpp
    NatsuLib/natEvent.h
    NatsuLib/natException.cpp
    NatsuLib/natException.h
    NatsuLib/natInterface.h
    NatsuLib/natLinq.h
    NatsuLib/natLocalFileScheme.cpp
    NatsuLib/natLocalFileScheme.h
    NatsuLib/natLog.cpp
    NatsuLib/natLog.h
    NatsuLib/natMat.h
    NatsuLib/natMath.h
    NatsuLib/natMisc.cpp
    NatsuLib/natMisc.h
    NatsuLib/natMultiThread.cpp
    NatsuLib/natMultiThread.h
    NatsuLib/natNamedPipe.cpp
    NatsuLib/natNamedPipe.h
    NatsuLib/natNode.h
    NatsuLib/natQuat.h
    NatsuLib/natRefObj.h
    NatsuLib/natStackWalker.cpp
    NatsuLib/natStackWalker.h
    NatsuLib/natStopWatch.cpp
    NatsuLib/natStopWatch.h
    NatsuLib/natStream.cpp
    NatsuLib/natStream.h
    NatsuLib/natStreamHelper.h
    NatsuLib/natString.cpp
    NatsuLib/natString.h
    NatsuLib/natStringUtil.h
    NatsuLib/natTask.cpp
    NatsuLib/natTask.h
    NatsuLib/natText.h
    NatsuLib/natTransform.h
    NatsuLib/natType.h
    NatsuLib/natUtil.cpp
    NatsuLib/natUtil.h
    NatsuLib/natVec.h
    NatsuLib/natVFS.cpp
    NatsuLib/natVFS.h
    NatsuLib/stdafx.cpp
    NatsuLib/stdafx.h
    NatsuLib/targetver.h)

add_library(NatsuLib STATIC ${SOURCE_FILES})
