FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/thirds
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoapps SHARED
	nemoenvs.c
	nemoglib.c
	nemocurl.c
	nemohttp.c
	showhelper.c
	${CMAKE_SOURCE_DIR}/thirds/http_parser.c
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
	nemoglib.h
	nemocurl.h
	nemohttp.h
	showhelper.h
	)

configure_file(
	nemoapps.pc.in
	nemoapps.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoapps)
INSTALL(FILES ${CMAKE_BINARY_DIR}/apps/nemoapps.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
