FIND_PACKAGE(PkgConfig REQUIRED)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/xdg-shell.xml
	xdg-shell
	)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/nemo-shell.xml
	nemo-shell
	)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/compz
	${CMAKE_BINARY_DIR}/compz
	${CMAKE_SOURCE_DIR}/shell
	${CMAKE_BINARY_DIR}/shell
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoshell SHARED
	shell.c
	grab.c
	busycursor.c
	move.c
	resize.c
	pick.c
	inputpanel.c
	waylandshell.c
	xdgshell.c
	nemoshell.c
	${PROTOCOLS}
	)

TARGET_LINK_LIBRARIES(nemoshell
	nemoasst
	nemocompz
	${NEMOUX_COMMON_LIBRARIES}
	)

set(NEMOSHELL_PKGCONFIG_REQUIRES "nemocompz")

INSTALL(TARGETS nemoshell DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

configure_file(
	nemoshell.pc.in
	nemoshell.pc
	@ONLY
	)

INSTALL(FILES ${CMAKE_BINARY_DIR}/shell/nemoshell.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
