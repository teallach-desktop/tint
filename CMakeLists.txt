cmake_minimum_required(VERSION 3.23)
project(tint VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")

find_package(Qt6 REQUIRED COMPONENTS Gui Widgets)
find_package(LayerShellQt REQUIRED)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
find_package (WaylandScanner REQUIRED)
ecm_add_wayland_client_protocol (
	WLR_FOREIGN_TOPLEVEL_MANAGEMENT_SRC
	PROTOCOL wlr-foreign-toplevel-management-unstable-v1.xml
	BASENAME wlr-foreign-toplevel-management
)
include_directories(${CMAKE_BINARY_DIR})

add_executable(tint
	main.cpp
	panel.cpp panel.h
	taskbar.cpp taskbar.h
	task.cpp task.h
	${WLR_FOREIGN_TOPLEVEL_MANAGEMENT_SRC}
)

target_link_options(tint
	BEFORE PUBLIC -fsanitize=undefined PUBLIC -fsanitize=address
)

target_link_libraries(tint
	PRIVATE
	LayerShellQtInterface
	Qt::Gui
	Qt::Widgets
	Qt::GuiPrivate
	wayland-client
)
