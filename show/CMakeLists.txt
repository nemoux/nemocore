FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/thirds
	)

if(WITH_SHOWEXPR)
	add_definitions(-DNEMOUX_WITH_SHOWEXPR=1)

	set(EXTRA_SOURCES ${EXTRA_SOURCES}
		showexpr.cpp
		)
endif(WITH_SHOWEXPR)

add_library(nemoshow SHARED
	nemoshow.cpp
	showone.cpp
	showscene.cpp
	showcanvas.cpp
	showitem.cpp
	showcolor.cpp
	showpath.cpp
	showpathcmd.cpp
	showsequence.cpp
	showease.cpp
	showtransition.cpp
	showmatrix.cpp
	showfilter.cpp
	showshader.cpp
	showfont.cpp
	showeasy.cpp
	showmisc.cpp
	showpoly.c
	showpipe.c
	showgrab.c
	svghelper.cpp
	skiahelper.cpp
	${EXTRA_SOURCES}
	)

TARGET_LINK_LIBRARIES(nemoshow
	nemoasst
	nemotale
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoshow DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/show/nemoshow.h
	${CMAKE_SOURCE_DIR}/show/showone.h
	${CMAKE_SOURCE_DIR}/show/showscene.h
	${CMAKE_SOURCE_DIR}/show/showcanvas.h
	${CMAKE_SOURCE_DIR}/show/showitem.h
	${CMAKE_SOURCE_DIR}/show/showitem.hpp
	${CMAKE_SOURCE_DIR}/show/showcolor.h
	${CMAKE_SOURCE_DIR}/show/showpath.h
	${CMAKE_SOURCE_DIR}/show/showpathcmd.h
	${CMAKE_SOURCE_DIR}/show/showsequence.h
	${CMAKE_SOURCE_DIR}/show/showease.h
	${CMAKE_SOURCE_DIR}/show/showtransition.h
	${CMAKE_SOURCE_DIR}/show/showmatrix.h
	${CMAKE_SOURCE_DIR}/show/showfilter.h
	${CMAKE_SOURCE_DIR}/show/showshader.h
	${CMAKE_SOURCE_DIR}/show/showexpr.h
	${CMAKE_SOURCE_DIR}/show/showfont.h
	${CMAKE_SOURCE_DIR}/show/showeasy.h
	${CMAKE_SOURCE_DIR}/show/showmisc.h
	${CMAKE_SOURCE_DIR}/show/showpoly.h
	${CMAKE_SOURCE_DIR}/show/showpipe.h
	${CMAKE_SOURCE_DIR}/show/showgrab.h
	${CMAKE_SOURCE_DIR}/show/showevent.h
	${CMAKE_SOURCE_DIR}/show/skiahelper.h
	${CMAKE_SOURCE_DIR}/show/skiaconfig.hpp
	${EXTRA_HEADERS}
	)

set(NEMOSHOW_PKGCONFIG_REQUIRES "nemoasst nemotale skia harfbuzz")

configure_file(
	nemoshow.pc.in
	nemoshow.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoshow)
INSTALL(FILES ${CMAKE_BINARY_DIR}/show/nemoshow.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
