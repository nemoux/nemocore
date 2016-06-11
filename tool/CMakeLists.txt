FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(WAYLANDCLIENT REQUIRED wayland-client)
INCLUDE_DIRECTORIES(${WAYLANDCLIENT_INCLUDE_DIRS})
LINK_DIRECTORIES(${WAYLANDCLIENT_LIBRARY_DIRS})

PKG_CHECK_MODULES(WAYLANDEGL REQUIRED wayland-egl)
INCLUDE_DIRECTORIES(${WAYLANDEGL_INCLUDE_DIRS})
LINK_DIRECTORIES(${WAYLANDEGL_LIBRARY_DIRS})

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
	)

if(WITH_HANGUL)
	PKG_CHECK_MODULES(LIBHANGUL REQUIRED libhangul)
	INCLUDE_DIRECTORIES(${LIBHANGUL_INCLUDE_DIRS})
	LINK_DIRECTORIES(${LIBHANGUL_LIBRARY_DIRS})

	set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES}
		${LIBHANGUL_LIBRARIES}
		)

	set(EXTRA_SOURCES ${EXTRA_SOURCES}
		nemohangul.c
		)

	set(EXTRA_HEADERS ${EXTRA_HEADERS}
		${CMAKE_SOURCE_DIR}/tool/nemohangul.h
		)
endif(WITH_HANGUL)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemotool SHARED
	nemotool.c
	nemocanvas.c
	nemooutput.c
	nemosound.c
	nemotimer.c
	nemomonitor.c
	nemoegl.c
	${PROTOCOLS}
	${EXTRA_SOURCES}
	)

TARGET_LINK_LIBRARIES(nemotool
	nemoasst
	${WAYLANDCLIENT_LIBRARIES}
	${WAYLANDEGL_LIBRARIES}
	${NEMOUX_COMMON_LIBRARIES}
	${EXTRA_LIBRARIES}
	)

set(NEMOTOOL_PKGCONFIG_REQUIRES "pixman-1 cairo expat wayland-client wayland-egl")

INSTALL(TARGETS nemotool DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/tool/nemotool.h
	${CMAKE_SOURCE_DIR}/tool/nemocanvas.h
	${CMAKE_SOURCE_DIR}/tool/nemooutput.h
	${CMAKE_SOURCE_DIR}/tool/nemosound.h
	${CMAKE_SOURCE_DIR}/tool/nemotimer.h
	${CMAKE_SOURCE_DIR}/tool/nemomonitor.h
	${CMAKE_SOURCE_DIR}/tool/nemoegl.h
	${CMAKE_BINARY_DIR}/tool/wayland-presentation-timing-client-protocol.h
	${CMAKE_BINARY_DIR}/tool/wayland-nemo-shell-client-protocol.h
	${CMAKE_BINARY_DIR}/tool/wayland-nemo-seat-client-protocol.h
	${CMAKE_BINARY_DIR}/tool/wayland-nemo-sound-client-protocol.h
	${EXTRA_HEADERS}
	)

configure_file(
	nemotool.pc.in
	nemotool.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemotool)
INSTALL(FILES ${CMAKE_BINARY_DIR}/tool/nemotool.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
