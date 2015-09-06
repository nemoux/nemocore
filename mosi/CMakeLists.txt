set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/mosi
	)

add_library(nemomosi SHARED
	nemomosi.c
	mosirandom.c
	mosiwave.c
	mosiflip.c
	)

set(NEMOMOSI_PKGCONFIG_REQUIRES "")

INSTALL(TARGETS nemomosi DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/mosi/nemomosi.h
	${CMAKE_SOURCE_DIR}/mosi/mosirandom.h
	${CMAKE_SOURCE_DIR}/mosi/mosiwave.h
	${CMAKE_SOURCE_DIR}/mosi/mosiflip.h
	)

configure_file(
	nemomosi.pc.in
	nemomosi.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemomosi)
INSTALL(FILES ${CMAKE_BINARY_DIR}/mosi/nemomosi.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
