FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(WAYLANDCLIENT REQUIRED wayland-client)
INCLUDE_DIRECTORIES(${WAYLANDCLIENT_INCLUDE_DIRS})
LINK_DIRECTORIES(${WAYLANDCLIENT_LIBRARY_DIRS})

wayland_add_protocol_client(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/presentation-timing.xml
	presentation-timing
	)

wayland_add_protocol_client(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/nemo-seat.xml
	nemo-seat
	)

wayland_add_protocol_client(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/nemo-sound.xml
	nemo-sound
	)

wayland_add_protocol_client(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/nemo-shell.xml
	nemo-shell
	)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/thirds
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

if(WITH_EGL)
	PKG_CHECK_MODULES(WAYLANDEGL REQUIRED wayland-egl)
	INCLUDE_DIRECTORIES(${WAYLANDEGL_INCLUDE_DIRS})
	LINK_DIRECTORIES(${WAYLANDEGL_LIBRARY_DIRS})

	set(EXTRA ${EXTRA}
		nemoegl.c
		)
endif(WITH_EGL)

add_library(nemotool SHARED
	nemotool.c
	nemocanvas.c
	nemooutput.c
	nemotimer.c
	nemocurl.c
	nemohttp.c
	${EXTRA}
	${PROTOCOLS}
	${CMAKE_SOURCE_DIR}/thirds/http_parser.c
	)

TARGET_LINK_LIBRARIES(nemotool
	nemoasst
	${WAYLANDCLIENT_LIBRARIES}
	${NEMOUX_COMMON_LIBRARIES}
	)

set(NEMOTOOL_PKGCONFIG_REQUIRES "pixman-1 cairo expat wayland-client wayland-egl")

if(WITH_EGL)
	TARGET_LINK_LIBRARIES(nemotool
		${WAYLANDEGL_LIBRARIES}
		)

	set(EXTRA_HEADERS ${EXTRA_HEADERS} ${CMAKE_SOURCE_DIR}/tool/nemoegl.h)
	set(NEMOTOOL_PKGCONFIG_REQUIRES "${NEMOTOOL_PKGCONFIG_REQUIRES} wayland-egl")
endif(WITH_EGL)

INSTALL(TARGETS nemotool DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/tool/nemotool.h
	${CMAKE_SOURCE_DIR}/tool/nemocanvas.h
	${CMAKE_SOURCE_DIR}/tool/nemooutput.h
	${CMAKE_SOURCE_DIR}/tool/nemotimer.h
	${CMAKE_SOURCE_DIR}/tool/nemocurl.h
	${EXTRA_HEADERS}
	)

configure_file(
	nemotool.pc.in
	nemotool.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemotool)
INSTALL(FILES ${CMAKE_BINARY_DIR}/tool/nemotool.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
