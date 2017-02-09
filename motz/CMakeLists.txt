FIND_PACKAGE(PkgConfig REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/helper
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/toyz
	)

add_library(nemomotz SHARED
	nemomotz.c
	motztrans.c
	motzobject.c
	motzburst.c
	)

TARGET_LINK_LIBRARIES(nemomotz
	nemoasst
	nemohelper
	nemotool
	nemotoyz
	)

INSTALL(TARGETS nemomotz DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/motz/nemomotz.h
	${CMAKE_SOURCE_DIR}/motz/motztrans.h
	${CMAKE_SOURCE_DIR}/motz/motzobject.h
	${CMAKE_SOURCE_DIR}/motz/motzburst.h
	)

set(NEMOMOTZ_PKGCONFIG_REQUIRES "nemoasst nemohelper nemotool nemotoyz")

configure_file(
	nemomotz.pc.in
	nemomotz.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemomotz)
INSTALL(FILES ${CMAKE_BINARY_DIR}/motz/nemomotz.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
