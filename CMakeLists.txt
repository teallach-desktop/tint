cmake_minimum_required(VERSION 3.23)
project(tint VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -g")

find_package(LayerShellQt REQUIRED)

set(PROTO "wlr-foreign-toplevel-management-unstable-v1")
add_custom_command(
	OUTPUT ${PROTO}-protocol.c ${PROTO}-client-protocol.h
	COMMAND wayland-scanner private-code ../${PROTO}.xml ${PROTO}-protocol.c
	COMMAND wayland-scanner client-header ../${PROTO}.xml ${PROTO}-client-protocol.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(tint
	main.cpp
	panel.cpp panel.h
	taskbar.cpp taskbar.h
	task.cpp task.h
	${PROTO}-protocol.c ${PROTO}-client-protocol.h
)

target_link_libraries(tint
	PRIVATE
	LayerShellQtInterface
	Qt::Gui
	wayland-client
)
