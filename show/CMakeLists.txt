FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tale
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemoshow SHARED
	nemoshow.c
	showone.c
	showscene.c
	)

TARGET_LINK_LIBRARIES(nemoshow
	nemoasst
	nemotale
	${NEMOUX_COMMON_LIBRARIES}
	)

INSTALL(TARGETS nemoshow DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/show/nemoshow.h
	${CMAKE_SOURCE_DIR}/show/showone.h
	${CMAKE_SOURCE_DIR}/show/showscene.h
	)

set(NEMOSHOW_PKGCONFIG_REQUIRES "nemoasst nemotale")

configure_file(
	nemoshow.pc.in
	nemoshow.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoshow)
INSTALL(FILES ${CMAKE_BINARY_DIR}/show/nemoshow.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
