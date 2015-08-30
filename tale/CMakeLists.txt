FIND_PACKAGE(PkgConfig REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	)

if(WITH_EGL)
	set(NEMOTALE_EGL_SOURCES
		talegl.c
		)

	set(NEMOTALE_EGL_HEADERS
		${CMAKE_SOURCE_DIR}/tale/talegl.h
		)
endif(WITH_EGL)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(nemotale SHARED
	nemotale.c
	talenode.c
	talepixman.c
	taleevent.c
	talegrab.c
	${NEMOTALE_EGL_SOURCES}
	)

TARGET_LINK_LIBRARIES(nemotale
	nemoasst
	${NEMOUX_COMMON_LIBRARIES}
	)

INSTALL(TARGETS nemotale DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/tale/nemotale.h
	${CMAKE_SOURCE_DIR}/tale/talenode.h
	${CMAKE_SOURCE_DIR}/tale/talepixman.h
	${CMAKE_SOURCE_DIR}/tale/taleevent.h
	${CMAKE_SOURCE_DIR}/tale/talegrab.h
	${CMAKE_SOURCE_DIR}/tale/talegesture.h
	${NEMOTALE_EGL_HEADERS}
	)

set(NEMOTALE_PKGCONFIG_REQUIRES "pixman-1 cairo expat")

if(WITH_EGL)
	set(NEMOTALE_PKGCONFIG_REQUIRES "${NEMOTALE_PKGCONFIG_REQUIRES} egl")
endif(WITH_EGL)

configure_file(
	nemotale.pc.in
	nemotale.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemotale)
INSTALL(FILES ${CMAKE_BINARY_DIR}/tale/nemotale.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
