FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemotale SHARED
	nemotale.c
	talenode.c
	talegl.c
	talepixman.c
	${CMAKE_SOURCE_DIR}/misc/glhelper.c
	${CMAKE_SOURCE_DIR}/misc/glcompz.c
	${CMAKE_SOURCE_DIR}/misc/fbohelper.c
	)

TARGET_LINK_LIBRARIES(nemotale
	nemoasst
	${NEMOUX_COMMON_LIBRARIES}
	)

INSTALL(TARGETS nemotale DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/tale/nemotale.h
	${CMAKE_SOURCE_DIR}/tale/talenode.h
	${CMAKE_SOURCE_DIR}/tale/talegl.h
	${CMAKE_SOURCE_DIR}/tale/talepixman.h
	)

set(NEMOTALE_PKGCONFIG_REQUIRES "pixman-1 cairo expat egl")

configure_file(
	nemotale.pc.in
	nemotale.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemotale)
INSTALL(FILES ${CMAKE_BINARY_DIR}/tale/nemotale.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
