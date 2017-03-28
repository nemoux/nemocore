FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(EXPAT REQUIRED expat)
INCLUDE_DIRECTORIES(${EXPAT_INCLUDE_DIRS})
LINK_DIRECTORIES(${EXPAT_LIBRARY_DIRS})

PKG_CHECK_MODULES(JSON REQUIRED json-c)
INCLUDE_DIRECTORIES(${JSON_INCLUDE_DIRS})
LINK_DIRECTORIES(${JSON_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/helper
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoasst SHARED
	nemoxml.c
	nemomatrix.c
	nemotoken.c
	nemoitem.c
	nemotable.c
	nemojson.c
	nemotransition.c
	nemonoty.c
	nemolog.c
	nemofs.c
	nemomisc.c
	)

TARGET_LINK_LIBRARIES(nemoasst
	${EXPAT_LIBRAIRES}
	${JSON_LIBRARIES}
	)

set(NEMOASST_PKGCONFIG_REQUIRES "expat json-c")

INSTALL(TARGETS nemoasst DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemoxml.h
	nemomatrix.h
	nemotoken.h
	nemoitem.h
	nemotable.h
	nemojson.h
	nemotransition.h
	nemonoty.h
	nemolog.h
	nemomisc.h
	nemoattr.h
	nemolist.h
	nemolistener.h
	nemoease.h
	nemofs.h
	nemoconfig.h
	)

configure_file(
	nemoasst.pc.in
	nemoasst.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoasst)
INSTALL(FILES ${CMAKE_BINARY_DIR}/asst/nemoasst.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
