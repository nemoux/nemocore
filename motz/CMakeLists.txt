FIND_PACKAGE(PkgConfig REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/helper
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/tozz
	)

add_library(nemomotz SHARED
	nemomotz.c
	motzgroup.c
	motzclip.c
	motzobject.c
	motzpath.c
	motzburst.c
	motzswirl.c
	)

TARGET_LINK_LIBRARIES(nemomotz
	nemoasst
	nemohelper
	nemotool
	nemotozz
	)

INSTALL(TARGETS nemomotz DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/motz/nemomotz.h
	${CMAKE_SOURCE_DIR}/motz/motzgroup.h
	${CMAKE_SOURCE_DIR}/motz/motzclip.h
	${CMAKE_SOURCE_DIR}/motz/motzobject.h
	${CMAKE_SOURCE_DIR}/motz/motzpath.h
	${CMAKE_SOURCE_DIR}/motz/motzburst.h
	${CMAKE_SOURCE_DIR}/motz/motzswirl.h
	)

set(NEMOMOTZ_PKGCONFIG_REQUIRES "nemoasst nemohelper nemotool nemotozz")

configure_file(
	nemomotz.pc.in
	nemomotz.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemomotz)
INSTALL(FILES ${CMAKE_BINARY_DIR}/motz/nemomotz.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
