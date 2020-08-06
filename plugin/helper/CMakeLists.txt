cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(hyamt)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/../kernel/stubs/
)

add_executable(hyamt.elf
	main.c
)

target_link_libraries(hyamt.elf
	SceModulemgrForDriver_stub
	SceThreadmgrForDriver_stub
	SceIofilemgrForDriver_stub
	SceSysrootForDriver_stub
	SceKernelSuspendForDriver_stub
	SceSysclibForDriver_stub
	SceSdifForDriver_stub
	SceSysmemForDriver_stub
	taihenForKernel_stub
	taihenModuleUtils_stub
	SceDebugForDriver_stub
)

set_target_properties(hyamt.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(hyamt.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/hyamt.yml hyamt.elf hyamt.velf
	COMMAND vita-make-fself -c hyamt.velf hyamt.skprx
)

add_dependencies(hyamt.skprx hyamt.elf)

vita_create_stubs(stubs hyamt.elf ${CMAKE_SOURCE_DIR}/hyamt.yml KERNEL)
