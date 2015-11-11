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
	${CMAKE_SOURCE_DIR}/thirds
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemolog
	nemolog.c
	)

TARGET_LINK_LIBRARIES(nemolog
	nemoasst
	)

INSTALL(TARGETS nemolog DESTINATION bin)

add_executable(nemotouch
	nemotouch.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(nemotouch
	nemoasst
	nemotool
	nemotale
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemotouch DESTINATION bin)

add_executable(nemoexec
	nemoexec.c
	)

TARGET_LINK_LIBRARIES(nemoexec
	nemoasst
	)

INSTALL(TARGETS nemoexec DESTINATION bin)

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
	nemoimage.c
	talehelper.c
	)

TARGET_LINK_LIBRARIES(nemoimage
	nemotool
	nemotale
	)

INSTALL(TARGETS nemoimage DESTINATION bin)

add_executable(nemomesh
	nemomesh.cpp
	meshhelper.cpp
	talehelper.c
	${CMAKE_SOURCE_DIR}/thirds/tiny_obj_loader.cc
	)

TARGET_LINK_LIBRARIES(nemomesh
	nemotool
	nemotale
	)

INSTALL(TARGETS nemomesh DESTINATION bin)

if(WITH_MUPDF)
	add_executable(nemopdf
		nemopdf.c
		pdfhelper.c
		talehelper.c
		)

	TARGET_LINK_LIBRARIES(nemopdf
		mupdf
		mujs
		muopenjpeg
		mujpeg
		mujbig2dec
		freetype
		crypto
		z
		nemotool
		nemotale
		)

	INSTALL(TARGETS nemopdf DESTINATION bin)
endif(WITH_MUPDF)

if(WITH_GSTREAMER AND WITH_PULSEAUDIO)
	PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0)
	INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GLIB_LIBRARY_DIRS})

	PKG_CHECK_MODULES(GST REQUIRED gstreamer-1.0)
	INCLUDE_DIRECTORIES(${GST_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GST_LIBRARY_DIRS})

	PKG_CHECK_MODULES(GSTPBUTILS REQUIRED gstreamer-pbutils-1.0)
	INCLUDE_DIRECTORIES(${GSTPBUTILS_INCLUDE_DIRS})
	LINK_DIRECTORIES(${GSTPBUTILS_LIBRARY_DIRS})

	add_executable(nemoplay
		nemoplay.c
		gsthelper.c
		talehelper.c
		)

	TARGET_LINK_LIBRARIES(nemoplay
		nemotool
		nemotale
		${GLIB_LIBRARIES}
		${GST_LIBRARIES}
		${GSTPBUTILS_LIBRARIES}
		${GLIBPULSE_LIBRARIES}
		)

	INSTALL(TARGETS nemoplay DESTINATION bin)
endif(WITH_GSTREAMER AND WITH_PULSEAUDIO)
