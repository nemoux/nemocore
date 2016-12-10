set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemohelper SHARED
	pixmanhelper.c
	bezierhelper.c
	codehelper.c
	utf8helper.c
	fonthelper.c
	stringhelper.c
	namespacehelper.c
	keycodehelper.c
	colorhelper.c
	syshelper.c
	udphelper.c
	fbhelper.c
	glhelper.c
	compzhelper.c
	cliphelper.c
	ttyhelper.c
	)

TARGET_LINK_LIBRARIES(nemohelper
	nemoasst
	${NEMOUX_COMMON_LIBRARIES}
	)

set(NEMOASST_PKGCONFIG_REQUIRES "pixman-1")

INSTALL(TARGETS nemohelper DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	pixmanhelper.h
	bezierhelper.h
	codehelper.h
	utf8helper.h
	fonthelper.h
	stringhelper.h
	namespacehelper.h
	keycodehelper.h
	colorhelper.h
	syshelper.h
	udphelper.h
	fbhelper.h
	glhelper.h
	compzhelper.h
	cliphelper.h
	ttyhelper.h
	nemohelper.h
	)

configure_file(
	nemohelper.pc.in
	nemohelper.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemohelper)
INSTALL(FILES ${CMAKE_BINARY_DIR}/helper/nemohelper.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
