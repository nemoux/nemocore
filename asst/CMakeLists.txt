set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoasst SHARED
	nemoxml.c
	nemomatrix.c
	nemohash.c
	nemotoken.c
	nemoitem.c
	nemotrans.c
	nemolog.c
	nemopool.c
	nemofs.c
	nemomisc.c
	)

TARGET_LINK_LIBRARIES(nemoasst
	${NEMOUX_COMMON_LIBRARIES}
	)

set(NEMOASST_PKGCONFIG_REQUIRES "expat json-c")

INSTALL(TARGETS nemoasst DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemoxml.h
	nemomatrix.h
	nemohash.h
	nemotoken.h
	nemoitem.h
	nemotrans.h
	nemolog.h
	nemomisc.h
	nemoattr.h
	nemolist.h
	nemolistener.h
	nemoease.h
	nemopool.h
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
