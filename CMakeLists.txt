# File author is Ítalo Lima Marconato Matias
#
# Created on November 28 of 2020, at 10:32 BRT
# Last edited on December 01 of 2020, at 12:59 BRT

cmake_minimum_required(VERSION 3.12.0)

# We need to set the module path before calling project(), as we have a custom platform file.

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../host-tools/cmake")
else()
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/host-tools/cmake")
endif()

project(kernel CXX ASM-ATT)

include(CHOSOptions)
include(${CHOS_ARCH}Specific)

set(CMAKE_CXX_LINK_EXECUTABLE
    "ld.lld <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> ${KERNEL_BEFORE} <OBJECTS> ${KERNEL_AFTER} -o <TARGET>")

set(KERNEL_SOURCES ${KERNEL_ARCH_SOURCES}
				   
				   misc/bytehelper.cxx
				   misc/string.cxx
				   misc/textout.cxx
				   
				   mm/heap.cxx
				   mm/pmm.cxx
				   
				   sys/cxxsup.cxx
				   sys/fs.cxx
				   sys/lock.cxx
				   sys/main.cxx
				   sys/math.cxx
				   sys/process.cxx
				   sys/siafs.cxx
				   
				   vid/console.cxx
				   vid/display.cxx
				   vid/fontdata.cxx
				   vid/image.cxx)
list(TRANSFORM KERNEL_SOURCES PREPEND ${PROJECT_SOURCE_DIR}/)

add_executable(chkrnl.elf ${KERNEL_SOURCES})

target_include_directories(chkrnl.elf PRIVATE ${PROJECT_SOURCE_DIR}/include
											  ${PROJECT_SOURCE_DIR}/arch/${CHOS_ARCH}/include)
target_compile_options(chkrnl.elf PRIVATE -target ${CLANG_KERNEL_TARGET} -fno-rtti -fno-pic -Wall -Wextra)
target_compile_options(chkrnl.elf PRIVATE $<$<NOT:$<COMPILE_LANGUAGE:ASM-ATT>>:-ffreestanding -funroll-loops -fno-exceptions
																			   -fno-use-cxa-atexit -fno-plt -fno-stack-protector
																			   -fno-use-init-array>)
target_compile_definitions(chkrnl.elf PRIVATE KERNEL VERSION="${CHOS_VERSION}" ARCH="${CHOS_ARCH}")
target_link_options(chkrnl.elf PRIVATE -T${PROJECT_SOURCE_DIR}/arch/${CHOS_ARCH}/link.ld)
setup_kernel_arch(chkrnl.elf)
