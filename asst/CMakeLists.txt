FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoasst SHARED
	nemomatrix.c
	nemotoken.c
	nemomemo.c
	nemostring.c
	nemoitem.c
	nemotable.c
	nemotransition.c
	nemonoty.c
	nemolog.c
	nemodick.c
	nemofs.c
	nemomisc.c
	)

set(NEMOASST_PKGCONFIG_REQUIRES "")

INSTALL(TARGETS nemoasst DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemomatrix.h
	nemotoken.h
	nemomemo.h
	nemostring.h
	nemoitem.h
	nemotable.h
	nemotransition.h
	nemonoty.h
	nemolog.h
	nemomisc.h
	nemoattr.h
	nemolist.h
	nemolistener.h
	nemodick.h
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
