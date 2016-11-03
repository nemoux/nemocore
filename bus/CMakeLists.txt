set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	)

add_library(nemobus SHARED
	nemobus.c
	)

TARGET_LINK_LIBRARIES(nemobus
	nemoasst
	)

INSTALL(TARGETS nemobus DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/bus/nemobus.h
	)

set(NEMOBUS_PKGCONFIG_REQUIRES "nemoasst")

configure_file(
	nemobus.pc.in
	nemobus.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemobus)
INSTALL(FILES ${CMAKE_BINARY_DIR}/bus/nemobus.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
