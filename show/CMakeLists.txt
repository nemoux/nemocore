FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/thirds
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoshow SHARED
	nemoshow.cpp
	showone.cpp
	showscene.cpp
	showcanvas.cpp
	showshape.cpp
	showcolor.cpp
	showexpr.cpp
	showloop.cpp
	showsequence.cpp
	showease.cpp
	showtransition.cpp
	showmisc.cpp
	)

TARGET_LINK_LIBRARIES(nemoshow
	nemoasst
	nemotale
	${NEMOUX_COMMON_LIBRARIES}
	)

INSTALL(TARGETS nemoshow DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/show/nemoshow.h
	${CMAKE_SOURCE_DIR}/show/showone.h
	${CMAKE_SOURCE_DIR}/show/showscene.h
	${CMAKE_SOURCE_DIR}/show/showcanvas.h
	${CMAKE_SOURCE_DIR}/show/showshape.h
	${CMAKE_SOURCE_DIR}/show/showcolor.h
	${CMAKE_SOURCE_DIR}/show/showexpr.h
	${CMAKE_SOURCE_DIR}/show/showloop.h
	${CMAKE_SOURCE_DIR}/show/showsequence.h
	${CMAKE_SOURCE_DIR}/show/showease.h
	${CMAKE_SOURCE_DIR}/show/showtransition.h
	)

set(NEMOSHOW_PKGCONFIG_REQUIRES "nemoasst nemotale")

configure_file(
	nemoshow.pc.in
	nemoshow.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoshow)
INSTALL(FILES ${CMAKE_BINARY_DIR}/show/nemoshow.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
