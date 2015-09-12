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

add_executable(miniback
	miniback.c
	talehelper.c
	)

TARGET_LINK_LIBRARIES(miniback
	nemotale
	nemotool
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

add_executable(meshback
	meshback.cpp
	talehelper.c
	${CMAKE_SOURCE_DIR}/thirds/tiny_obj_loader.cc
	)

TARGET_LINK_LIBRARIES(meshback
	nemotale
	nemotool
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

add_executable(plexback
	plexback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(plexback
	nemotool
	nemotale
	poly2tri
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

add_executable(moteback
	moteback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(moteback
	nemotool
	nemotale
	nemomote
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

add_executable(mosiback
	mosiback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(mosiback
	nemotool
	nemotale
	nemomosi
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

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

	add_executable(playback
		playback.c
		talehelper.c
		${CMAKE_SOURCE_DIR}/apps/gsthelper.c
		${CMAKE_SOURCE_DIR}/apps/glibhelper.c
		)

	TARGET_LINK_LIBRARIES(playback
		nemotool
		nemotale
		${GLIB_LIBRARIES}
		${GST_LIBRARIES}
		${GSTPBUTILS_LIBRARIES}
		)
endif(WITH_GSTREAMER)
