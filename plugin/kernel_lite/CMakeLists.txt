cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(yamt)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

add_executable(yamt.elf
	tai_compat.c
	main.c
)

target_link_libraries(yamt.elf
	SceThreadmgrForDriver_stub
	SceSysconForDriver_stub
	SceIofilemgrForDriver_stub
	SceSdifForDriver_stub
	SceSysclibForDriver_stub
	SceCpuForDriver_stub
	SceModulemgrForDriver_stub
	ScePervasiveForDriver_stub
	SceSysrootForKernel_stub
	SceSysmemForDriver_stub
)

set_target_properties(yamt.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(yamt.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/yamt.yml yamt.elf yamt.velf
	COMMAND vita-make-fself -c yamt.velf yamt.skprx
)

add_dependencies(yamt.skprx yamt.elf)
