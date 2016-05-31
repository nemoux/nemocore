FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/misc
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemomsg SHARED
	nemomsg.c
	msgqueue.c
	msgconn.c
	)

TARGET_LINK_LIBRARIES(nemomsg
	${NEMOUX_COMMON_LIBRARIES}
	)

set(NEMOASST_PKGCONFIG_REQUIRES "nemoasst")

INSTALL(TARGETS nemomsg DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemomsg.h
	msgqueue.h
	msgconn.h
	)

configure_file(
	nemomsg.pc.in
	nemomsg.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemomsg)
INSTALL(FILES ${CMAKE_BINARY_DIR}/msg/nemomsg.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
