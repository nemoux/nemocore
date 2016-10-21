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
	nemomsg.c
	nemopoly.c
	nemospin.c
	nemoscope.c
	nemoclip.c
	nemotrix.c
	nemopool.c
	nemokeys.c
	nemonote.c
	nemomisc.c
	${CMAKE_SOURCE_DIR}/misc/xmlparser.c
	${CMAKE_SOURCE_DIR}/misc/pixmanhelper.c
	${CMAKE_SOURCE_DIR}/misc/bezierhelper.c
	${CMAKE_SOURCE_DIR}/misc/codehelper.c
	${CMAKE_SOURCE_DIR}/misc/utf8helper.c
	${CMAKE_SOURCE_DIR}/misc/fonthelper.c
	${CMAKE_SOURCE_DIR}/misc/stringhelper.c
	${CMAKE_SOURCE_DIR}/misc/namespacehelper.c
	${CMAKE_SOURCE_DIR}/misc/keycodehelper.c
	${CMAKE_SOURCE_DIR}/misc/colorhelper.c
	${CMAKE_SOURCE_DIR}/misc/oshelper.c
	${CMAKE_SOURCE_DIR}/misc/syshelper.c
	${CMAKE_SOURCE_DIR}/misc/udphelper.c
	)

TARGET_LINK_LIBRARIES(nemoasst
	${NEMOUX_COMMON_LIBRARIES}
	)

set(NEMOASST_PKGCONFIG_REQUIRES "pixman-1 cairo expat json-c")

INSTALL(TARGETS nemoasst DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemoxml.h
	nemomatrix.h
	nemohash.h
	nemotoken.h
	nemoitem.h
	nemotrans.h
	nemolog.h
	nemomsg.h
	nemomisc.h
	nemoattr.h
	nemolist.h
	nemolistener.h
	nemoease.h
	nemopoly.h
	nemospin.h
	nemoscope.h
	nemoclip.h
	nemotrix.h
	nemopool.h
	nemokeys.h
	nemonote.h
	nemoconfig.h
	nemohelper.h
	)

configure_file(
	nemoasst.pc.in
	nemoasst.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoasst)
INSTALL(FILES ${CMAKE_BINARY_DIR}/asst/nemoasst.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
