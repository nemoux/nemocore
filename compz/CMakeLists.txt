FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(PIXMAN REQUIRED pixman-1)
INCLUDE_DIRECTORIES(${PIXMAN_INCLUDE_DIRS})
LINK_DIRECTORIES(${PIXMAN_LIBRARY_DIRS})

PKG_CHECK_MODULES(WAYLANDCURSOR REQUIRED wayland-cursor)
INCLUDE_DIRECTORIES(${WAYLANDCURSOR_INCLUDE_DIRS})
LINK_DIRECTORIES(${WAYLANDCURSOR_LIBRARY_DIRS})

PKG_CHECK_MODULES(WAYLANDSERVER REQUIRED wayland-server)
INCLUDE_DIRECTORIES(${WAYLANDSERVER_INCLUDE_DIRS})
LINK_DIRECTORIES(${WAYLANDSERVER_LIBRARY_DIRS})

PKG_CHECK_MODULES(MTDEV REQUIRED mtdev)
INCLUDE_DIRECTORIES(${MTDEV_INCLUDE_DIRS})
LINK_DIRECTORIES(${MTDEV_LIBRARY_DIRS})

PKG_CHECK_MODULES(LIBUDEV REQUIRED libudev)
INCLUDE_DIRECTORIES(${LIBUDEV_INCLUDE_DIRS})
LINK_DIRECTORIES(${LIBUDEV_LIBRARY_DIRS})

PKG_CHECK_MODULES(LIBLO REQUIRED liblo)
INCLUDE_DIRECTORIES(${LIBLO_INCLUDE_DIRS})
LINK_DIRECTORIES(${LIBLO_LIBRARY_DIRS})

PKG_CHECK_MODULES(GL REQUIRED gl)
INCLUDE_DIRECTORIES(${GL_INCLUDE_DIRS})
LINK_DIRECTORIES(${GL_LIBRARY_DIRS})

PKG_CHECK_MODULES(EGL REQUIRED egl)
INCLUDE_DIRECTORIES(${EGL_INCLUDE_DIRS})
LINK_DIRECTORIES(${EGL_LIBRARY_DIRS})

PKG_CHECK_MODULES(WAYLANDEGL REQUIRED wayland-egl)
INCLUDE_DIRECTORIES(${WAYLANDEGL_INCLUDE_DIRS})
LINK_DIRECTORIES(${WAYLANDEGL_LIBRARY_DIRS})

PKG_CHECK_MODULES(LIBDRM REQUIRED libdrm)
INCLUDE_DIRECTORIES(${LIBDRM_INCLUDE_DIRS})
LINK_DIRECTORIES(${LIBDRM_LIBRARY_DIRS})

PKG_CHECK_MODULES(GBM REQUIRED gbm)
INCLUDE_DIRECTORIES(${GBM_INCLUDE_DIRS})
LINK_DIRECTORIES(${GBM_LIBRARY_DIRS})

set(WAYLAND_COMPOSITOR_VERSION "4" CACHE STRING "Wayland default compositor version")
set(DRM_PAGEFLIP_TIMEOUT "1000" CACHE STRING "DRM default pageflip timeout")

option(WITH_TOUCH_TO_POINTER "Build with Touch to Pointer" OFF)
option(WITH_LOGIND "Build with LOGIND" OFF)

include(wayland)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/scaler.xml
	scaler
	)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/text.xml
	text
	)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/input-method.xml
	input-method
	)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/presentation-timing.xml
	presentation-timing
	)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/nemo-seat.xml
	nemo-seat
	)

wayland_add_protocol_server(PROTOCOLS
	${CMAKE_SOURCE_DIR}/protocol/nemo-sound.xml
	nemo-sound
	)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/kits
	${CMAKE_SOURCE_DIR}/helper
	${CMAKE_SOURCE_DIR}/compz
	${CMAKE_BINARY_DIR}/compz
	)

if(WITH_TOUCH_TO_POINTER)
	add_definitions(-DNEMOUX_WITH_TOUCH_TO_POINTER=1)
endif(WITH_TOUCH_TO_POINTER)

if(WITH_LOGIND)
	add_definitions(-DNEMOUX_WITH_LOGIND=1)

	PKG_CHECK_MODULES(DBUS REQUIRED dbus-1)
	INCLUDE_DIRECTORIES(${DBUS_INCLUDE_DIRS})
	LINK_DIRECTORIES(${DBUS_LIBRARY_DIRS})

	PKG_CHECK_MODULES(LOGIND REQUIRED libsystemd-login)
	INCLUDE_DIRECTORIES(${LOGIND_INCLUDE_DIRS})
	LINK_DIRECTORIES(${LOGIND_LIBRARY_DIRS})

	set(EXTRA ${EXTRA}
		dbushelper.c
		logindhelper.c
		)
endif(WITH_LOGIND)

add_definitions(-DNEMOUX_WAYLAND_COMPOSITOR_VERSION=${WAYLAND_COMPOSITOR_VERSION})
add_definitions(-DNEMOUX_DRM_PAGEFLIP_TIMEOUT=${DRM_PAGEFLIP_TIMEOUT})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemocompz SHARED
	compz.c
	canvas.c
	scaler.c
	presentation.c
	region.c
	scope.c
	subcompz.c
	subcanvas.c
	view.c
	content.c
	layer.c
	session.c
	timer.c
	seat.c
	input.c
	keyboard.c
	pointer.c
	touch.c
	keypad.c
	datadevice.c
	dataoffer.c
	datadrag.c
	dataselection.c
	clipboard.c
	screen.c
	renderer.c
	fbbackend.c
	fbnode.c
	pixmanrenderer.c
	pixmancanvas.c
	glrenderer.c
	glcanvas.c
	drmbackend.c
	drmnode.c
	evdevbackend.c
	evdevnode.c
	tuio.c
	virtuio.c
	sound.c
	picker.c
	binding.c
	keymap.c
	backend.c
	waylandhelper.c
	xmlhelper.c
	${EXTRA}
	${PROTOCOLS}
	)

TARGET_LINK_LIBRARIES(nemocompz
	pthread
	${PIXMAN_LIBRARIES}
	${WAYLANDCURSOR_LIBRARIES}
	${WAYLANDSERVER_LIBRARIES}
	${MTDEV_LIBRARIES}
	${LIBUDEV_LIBRARIES}
	${LIBLO_LIBRARIES}
	nemoasst
	nemokits
	nemohelper
	)

set(NEMOCOMPZ_PKGCONFIG_REQUIRES "nemoasst nemohelper pixman-1 expat libudev wayland-cursor wayland-server")

TARGET_LINK_LIBRARIES(nemocompz
	${GL_LIBRARIES}
	${EGL_LIBRARIES}
	${WAYLANDEGL_LIBRARIES}
	)

set(NEMOCOMPZ_PKGCONFIG_REQUIRES "${NEMOCOMPZ_PKGCONFIG_REQUIRES} egl wayland-egl")

TARGET_LINK_LIBRARIES(nemocompz
	${LIBDRM_LIBRARIES}
	${GBM_LIBRARIES}
	)

set(NEMOCOMPZ_PKGCONFIG_REQUIRES "${NEMOCOMPZ_PKGCONFIG_REQUIRES} libdrm gbm")

if(WITH_LOGIND)
	TARGET_LINK_LIBRARIES(nemocompz
		${DBUS_LIBRARIES}
		${LOGIND_LIBRARIES}
		)

	set(NEMOCOMPZ_PKGCONFIG_REQUIRES "${NEMOCOMPZ_PKGCONFIG_REQUIRES} dbus-1 systemd")
endif(WITH_LOGIND)

INSTALL(TARGETS nemocompz DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/compz/compz.h
	${CMAKE_SOURCE_DIR}/compz/layer.h
	${CMAKE_SOURCE_DIR}/compz/backend.h
	${CMAKE_SOURCE_DIR}/compz/drmbackend.h
	${CMAKE_SOURCE_DIR}/compz/evdevbackend.h
	${CMAKE_SOURCE_DIR}/compz/renderer.h
	${CMAKE_SOURCE_DIR}/compz/view.h
	${CMAKE_SOURCE_DIR}/compz/content.h
	${CMAKE_SOURCE_DIR}/compz/canvas.h
	${CMAKE_SOURCE_DIR}/compz/subcanvas.h
	${CMAKE_SOURCE_DIR}/compz/screen.h
	${CMAKE_SOURCE_DIR}/compz/input.h
	${CMAKE_SOURCE_DIR}/compz/seat.h
	${CMAKE_SOURCE_DIR}/compz/keymap.h
	${CMAKE_SOURCE_DIR}/compz/keyboard.h
	${CMAKE_SOURCE_DIR}/compz/keypad.h
	${CMAKE_SOURCE_DIR}/compz/sound.h
	${CMAKE_SOURCE_DIR}/compz/picker.h
	${CMAKE_SOURCE_DIR}/compz/pointer.h
	${CMAKE_SOURCE_DIR}/compz/touch.h
	${CMAKE_SOURCE_DIR}/compz/tuio.h
	${CMAKE_SOURCE_DIR}/compz/virtuio.h
	${CMAKE_SOURCE_DIR}/compz/datadevice.h
	${CMAKE_SOURCE_DIR}/compz/session.h
	${CMAKE_SOURCE_DIR}/compz/binding.h
	${CMAKE_SOURCE_DIR}/compz/timer.h
	${CMAKE_SOURCE_DIR}/compz/animation.h
	${CMAKE_SOURCE_DIR}/compz/effect.h
	)

configure_file(
	nemocompz.pc.in
	nemocompz.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemocompz)
INSTALL(FILES ${CMAKE_BINARY_DIR}/compz/nemocompz.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
