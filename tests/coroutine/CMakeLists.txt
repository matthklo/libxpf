CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(libxpf)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include")





ADD_EXECUTABLE(coroutine_test
    coroutine_test.cpp
)
SET_PROPERTY(TARGET coroutine_test PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin")
IF(WIN32)
  ADD_DEFINITIONS(-DUNICODE -D_UNICODE)  
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(coroutine_test xpf)

