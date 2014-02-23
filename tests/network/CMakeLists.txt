CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(libxpf)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/external/STLport-5.2.1/stlport")





ADD_EXECUTABLE(network_test
    main.cpp
	sync_server.cpp
	sync_server.h
	sync_client.cpp
	sync_client.h
	async_server.cpp
	async_server.h
	async_client.cpp
	async_client.h
)
SET_PROPERTY(TARGET network_test PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin")
IF(WIN32)
  ADD_DEFINITIONS(-DUNICODE -D_UNICODE)  
ENDIF(WIN32)
TARGET_LINK_LIBRARIES(network_test xpf)

