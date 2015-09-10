set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/thirds
	/usr/include/skia/core
	/usr/include/skia/config
	/usr/include/skia/animator
	/usr/include/skia/codec
	/usr/include/skia/device
	/usr/include/skia/effects
	/usr/include/skia/images
	/usr/include/skia/pathops
	/usr/include/skia/pipe
	/usr/include/skia/ports
	/usr/include/skia/svg
	/usr/include/skia/utils
	/usr/include/skia/views
	/usr/include/skia/xml
	)

link_directories(
	/usr/lib64/skia
	)

add_library(nemoshow SHARED
	nemoshow.cpp
	showone.cpp
	showscene.cpp
	showcanvas.cpp
	showitem.cpp
	showcolor.cpp
	showpath.cpp
	showexpr.cpp
	showsvg.cpp
	showsequence.cpp
	showease.cpp
	showtransition.cpp
	showmatrix.cpp
	showfilter.cpp
	showshader.cpp
	showcamera.cpp
	showcons.cpp
	showfont.cpp
	showeasy.cpp
	showhelper.cpp
	showmisc.cpp
	svghelper.cpp
	)

TARGET_LINK_LIBRARIES(nemoshow
	nemoasst
	nemotale
	skia
	${NEMOUX_COMMON_LIBRARIES}
	)

INSTALL(TARGETS nemoshow DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/show/nemoshow.h
	${CMAKE_SOURCE_DIR}/show/showone.h
	${CMAKE_SOURCE_DIR}/show/showscene.h
	${CMAKE_SOURCE_DIR}/show/showcanvas.h
	${CMAKE_SOURCE_DIR}/show/showitem.h
	${CMAKE_SOURCE_DIR}/show/showcolor.h
	${CMAKE_SOURCE_DIR}/show/showpath.h
	${CMAKE_SOURCE_DIR}/show/showexpr.h
	${CMAKE_SOURCE_DIR}/show/showsvg.h
	${CMAKE_SOURCE_DIR}/show/showsequence.h
	${CMAKE_SOURCE_DIR}/show/showease.h
	${CMAKE_SOURCE_DIR}/show/showtransition.h
	${CMAKE_SOURCE_DIR}/show/showmatrix.h
	${CMAKE_SOURCE_DIR}/show/showfilter.h
	${CMAKE_SOURCE_DIR}/show/showshader.h
	${CMAKE_SOURCE_DIR}/show/showcamera.h
	${CMAKE_SOURCE_DIR}/show/showcons.h
	${CMAKE_SOURCE_DIR}/show/showfont.h
	${CMAKE_SOURCE_DIR}/show/showeasy.h
	)

set(NEMOSHOW_PKGCONFIG_REQUIRES "nemoasst nemotale")

configure_file(
	nemoshow.pc.in
	nemoshow.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoshow)
INSTALL(FILES ${CMAKE_BINARY_DIR}/show/nemoshow.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
