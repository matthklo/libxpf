CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(libxpf)

FILE(GLOB EXPORTED_HEADER_FILES "${CMAKE_SOURCE_DIR}/include/xpf/*.h")
FILE(GLOB INTERNAL_HEADER_FILES "${CMAKE_SOURCE_DIR}/src/platform/*.hpp")
FILE(GLOB INTERNAL_SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
SOURCE_GROUP("Exported Headers" FILES ${EXPORTED_HEADER_FILES})
SOURCE_GROUP("Internal Headers" FILES ${INTERNAL_HEADER_FILES})

IF(NOT WIN32)
  FIND_PACKAGE(Threads)
ENDIF(NOT WIN32)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/include")


IF(WIN32)
  ENABLE_LANGUAGE(ASM_MASM)
  IF(CMAKE_CL_64)
    ADD_CUSTOM_COMMAND(OUTPUT jump_x86_64_ms_pe_masm.obj COMMAND ${CMAKE_ASM_MASM_COMPILER} -c ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_x86_64_ms_pe_masm.asm DEPENDS ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_x86_64_ms_pe_masm.asm COMMENT "generate jump_x86_64_ms_pe_masm.obj" )
    ADD_CUSTOM_COMMAND(OUTPUT make_x86_64_ms_pe_masm.obj COMMAND ${CMAKE_ASM_MASM_COMPILER} -c ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_x86_64_ms_pe_masm.asm DEPENDS ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_x86_64_ms_pe_masm.asm COMMENT "generate make_x86_64_ms_pe_masm.obj" )
    SET(ASM_FILES jump_x86_64_ms_pe_masm.obj make_x86_64_ms_pe_masm.obj)
  ELSE()
    ADD_CUSTOM_COMMAND(OUTPUT jump_i386_ms_pe_masm.obj COMMAND ${CMAKE_ASM_MASM_COMPILER} -c ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_i386_ms_pe_masm.asm DEPENDS ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_i386_ms_pe_masm.asm COMMENT "generate jump_i386_ms_pe_masm.obj" )
    ADD_CUSTOM_COMMAND(OUTPUT make_i386_ms_pe_masm.obj COMMAND ${CMAKE_ASM_MASM_COMPILER} -c ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_i386_ms_pe_masm.asm DEPENDS ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_i386_ms_pe_masm.asm COMMENT "generate make_i386_ms_pe_masm.obj" )
    SET(ASM_FILES jump_i386_ms_pe_masm.obj make_i386_ms_pe_masm.obj)
  ENDIF()
ELSEIF(APPLE)
  ENABLE_LANGUAGE(ASM)
ELSE()
  ENABLE_LANGUAGE(ASM)
  IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL i386)
    SET(ASM_FILES ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_i386_sysv_elf_gas.S
                  ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_i386_sysv_elf_gas.S)
  ELSEIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL x86_64)
    SET(ASM_FILES ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_x86_64_sysv_elf_gas.S
                  ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_x86_64_sysv_elf_gas.S)
  ELSEIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL mips)
    SET(ASM_FILES ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_mips32_o32_elf_gas.S
                  ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_mips32_o32_elf_gas.S)
  ELSEIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL arm)
    SET(ASM_FILES ${CMAKE_SOURCE_DIR}/external/boost/context/asm/jump_arm_aapcs_elf_gas.S
                  ${CMAKE_SOURCE_DIR}/external/boost/context/asm/make_arm_aapcs_elf_gas.S)
  ELSE()
  ENDIF()
ENDIF()



ADD_LIBRARY(xpf SHARED
  ${EXPORTED_HEADER_FILES}
  ${INTERNAL_HEADER_FILES}
  ${INTERNAL_SRC_FILES}
  ${ASM_FILES}
)
SET_PROPERTY(TARGET xpf PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib")
SET_PROPERTY(TARGET xpf PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin")
SET_PROPERTY(TARGET xpf PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin")
SET_PROPERTY(TARGET xpf PROPERTY DEFINE_SYMBOL XPF_BUILD_LIBRARY)
IF(WIN32)
  ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
  TARGET_LINK_LIBRARIES(xpf ${CMAKE_THREAD_LIBS_INIT} ws2_32)
ELSE(WIN32)
  TARGET_LINK_LIBRARIES(xpf ${CMAKE_THREAD_LIBS_INIT})
ENDIF(WIN32)






ADD_LIBRARY(xpf_static STATIC
  ${EXPORTED_HEADER_FILES}
  ${INTERNAL_HEADER_FILES}
  ${INTERNAL_SRC_FILES}
  ${ASM_FILES}
)
SET_PROPERTY(TARGET xpf_static PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib")
SET_PROPERTY(TARGET xpf_static PROPERTY APPEND_STRING PROPERTY COMPILE_DEFINITIONS XPF_BUILD_STATIC_LIBRARY)
IF(WIN32)
  ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
ENDIF(WIN32)
