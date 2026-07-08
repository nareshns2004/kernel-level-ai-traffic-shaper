cmake_minimum_required(VERSION 3.16)
project(kernelmind-shaper VERSION 0.1.0 LANGUAGES C)

option(BUILD_KERNEL "Build kernel module via make" ON)
option(BUILD_USERSPACE "Build userspace tools via make" ON)

if(BUILD_KERNEL)
	add_custom_target(kernel_module
		COMMAND ${CMAKE_MAKE_PROGRAM} kernel
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Building kernelmind.ko"
	)
endif()

if(BUILD_USERSPACE)
	add_custom_target(userspace_tools
		COMMAND ${CMAKE_MAKE_PROGRAM} userspace
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Building kernelmind CLI and kmshaperd"
	)
endif()

add_custom_target(kernelmind_all
	COMMAND ${CMAKE_MAKE_PROGRAM} all
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(kernelmind_test
	COMMAND ${CMAKE_MAKE_PROGRAM} test
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

install(PROGRAMS scripts/install.sh scripts/setup_htb.sh
	DESTINATION bin
)
