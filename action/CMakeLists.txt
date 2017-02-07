FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/helper
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoaction SHARED
	nemoaction.c
	actionone.c
	actiontap.c
	)

TARGET_LINK_LIBRARIES(nemoaction
	nemoasst
	nemohelper
	)

INSTALL(TARGETS nemoaction DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/action/nemoaction.h
	${CMAKE_SOURCE_DIR}/action/actionone.h
	${CMAKE_SOURCE_DIR}/action/actiontap.h
	)

set(NEMOACTION_PKGCONFIG_REQUIRES "nemoasst nemohelper")

configure_file(
	nemoaction.pc.in
	nemoaction.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoaction)
INSTALL(FILES ${CMAKE_BINARY_DIR}/action/nemoaction.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
