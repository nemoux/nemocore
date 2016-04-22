FIND_PACKAGE(PkgConfig REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	)

add_library(nemonavi SHARED
	nemonavi.c
	naviclient.cpp
	)

TARGET_LINK_LIBRARIES(nemonavi
	nemoasst
	cef
	cef_dll_wrapper
	ssp
	)

INSTALL(TARGETS nemonavi DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/navi/nemonavi.h
	)

set(NEMOSHOW_PKGCONFIG_REQUIRES "nemoasst")

configure_file(
	nemonavi.pc.in
	nemonavi.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemonavi)
INSTALL(FILES ${CMAKE_BINARY_DIR}/navi/nemonavi.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
