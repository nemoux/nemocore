FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/mote
	${CMAKE_SOURCE_DIR}/mosi
	${CMAKE_SOURCE_DIR}/apps
	${CMAKE_SOURCE_DIR}/thirds
	${CMAKE_SOURCE_DIR}/thirds/poly2tri
	)

add_executable(nemoback-mini
	miniback.c
	talehelper.c
	)

TARGET_LINK_LIBRARIES(nemoback-mini
	nemotale
	nemotool
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-mini DESTINATION bin)

add_executable(nemoback-plex
	plexback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(nemoback-plex
	nemotool
	nemotale
	poly2tri
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-plex DESTINATION bin)

add_executable(nemoback-mote
	moteback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(nemoback-mote
	nemotool
	nemotale
	nemomote
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-mote DESTINATION bin)

add_executable(nemoback-mosi
	mosiback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(nemoback-mosi
	nemotool
	nemotale
	nemomosi
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-mosi DESTINATION bin)

if(WITH_GSTREAMER)
	PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0)
	INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GLIB_LIBRARY_DIRS})

	PKG_CHECK_MODULES(GST REQUIRED gstreamer-1.0)
	INCLUDE_DIRECTORIES(${GST_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GST_LIBRARY_DIRS})

	PKG_CHECK_MODULES(GSTPBUTILS REQUIRED gstreamer-pbutils-1.0)
	INCLUDE_DIRECTORIES(${GSTPBUTILS_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GSTPBUTILS_LIBRARY_DIRS})

	add_executable(nemoback-play
		playback.c
		talehelper.c
		${CMAKE_SOURCE_DIR}/apps/gsthelper.c
		${CMAKE_SOURCE_DIR}/apps/glibhelper.c
		)

	TARGET_LINK_LIBRARIES(nemoback-play
		nemotool
		nemotale
		${GLIB_LIBRARIES}
		${GST_LIBRARIES}
		${GSTPBUTILS_LIBRARIES}
		)

	INSTALL(TARGETS nemoback-play DESTINATION bin)
endif(WITH_GSTREAMER)
