FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(DBUS REQUIRED dbus-1)
INCLUDE_DIRECTORIES(${DBUS_INCLUDE_DIRS})
LINK_DIRECTORIES(${DBUS_LIBRARY_DIRS})

PKG_CHECK_MODULES(JSON REQUIRED json-c)
INCLUDE_DIRECTORIES(${JSON_INCLUDE_DIRS})
LINK_DIRECTORIES(${JSON_LIBRARY_DIRS})

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoapps SHARED
	nemoenvs.c
	talehelper.c
	showhelper.c
	)

TARGET_LINK_LIBRARIES(nemoapps
	nemoasst
	nemotool
	nemotale
	nemoshow
	)

set(NEMOAPPS_PKGCONFIG_REQUIRES "nemoasst nemotool nemotale nemoshow")

INSTALL(TARGETS nemoapps DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemoenvs.h
	talehelper.h
	showhelper.h
	)

configure_file(
	nemoapps.pc.in
	nemoapps.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoapps)
INSTALL(FILES ${CMAKE_BINARY_DIR}/apps/nemoapps.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)

add_executable(nemolog
	nemolog.c
	)

TARGET_LINK_LIBRARIES(nemolog
	nemoasst
	)

INSTALL(TARGETS nemolog DESTINATION bin)

if(WITH_PULSEAUDIO)
	PKG_CHECK_MODULES(LIBPULSE REQUIRED libpulse)
	INCLUDE_DIRECTORIES(${LIBPULSE_INCLUDE_DIRS})
	LINK_DIRECTORIES(${LIBPULSE_LIBRARY_DIRS})

	add_executable(nemopulse
		nemopulse.c
		)

	TARGET_LINK_LIBRARIES(nemopulse
		nemoasst
		${LIBPULSE_LIBRARIES}
		)

	INSTALL(TARGETS nemopulse DESTINATION bin)
endif(WITH_PULSEAUDIO)

add_executable(nemoimage
	nemoimage.cpp
	)

TARGET_LINK_LIBRARIES(nemoimage
	nemoapps
	)

INSTALL(TARGETS nemoimage DESTINATION bin)
