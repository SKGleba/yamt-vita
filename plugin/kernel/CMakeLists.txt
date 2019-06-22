cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(HENkaku)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}/365_stubs
)

IF(NEWFW)
	add_definitions(-DFW365)
ENDIF(NEWFW)

add_executable(yamt.elf
	tai_compat.c
	main.c
)

IF(NEWFW)
target_link_libraries(yamt.elf
	SceThreadmgrForDriver_stub
	SceSysconForDriver_stub
	SceIofilemgrForDriver_stub
	SceSdifForDriver_stub
	SceSysclibForDriver_stub
	SceCpuForDriver_stub
	SceModulemgrForKernel_367_stub
	ScePervasiveForDriver_stub
	SceSysrootForKernel_stub
)
ELSE(NEWFW)
target_link_libraries(yamt.elf
	SceThreadmgrForDriver_stub
	SceSysconForDriver_stub
	SceIofilemgrForDriver_stub
	SceSdifForDriver_stub
	SceSysclibForDriver_stub
	SceCpuForDriver_stub
	SceModulemgrForKernel_stub
	ScePervasiveForDriver_stub
	SceSysrootForKernel_stub
)
ENDIF(NEWFW)

set_target_properties(yamt.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(yamt.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/yamt.yml yamt.elf yamt.velf
	COMMAND vita-make-fself -c yamt.velf yamt.skprx
)
add_dependencies(yamt.skprx yamt.elf)