cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-vita-eabi-g++")

project(usbuo)

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/../kernel/stubs/
)

add_executable(usbuo.elf
	main.c
)

target_link_libraries(usbuo.elf
	kyamt_stub
	SceCpuForKernel_stub
	SceModulemgrForDriver_stub
	SceThreadmgrForDriver_stub
	SceIofilemgrForDriver_stub
	SceSysrootForDriver_stub
	SceKernelSuspendForDriver_stub
	SceSysclibForDriver_stub
)

set_target_properties(usbuo.elf
	PROPERTIES LINK_FLAGS "-nostdlib"
	COMPILE_FLAGS "-D__VITA_KERNEL__"
)

add_custom_target(usbuo.skprx ALL
	COMMAND vita-elf-create -e ${CMAKE_SOURCE_DIR}/usbuo.yml usbuo.elf usbuo.velf
	COMMAND vita-make-fself -c usbuo.velf usbuo.skprx
)
add_dependencies(usbuo.skprx usbuo.elf)
