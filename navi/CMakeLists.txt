option(WITH_NEMONAVI_DEBUG "Build with NEMONAVI DEBUG" OFF)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

if(WITH_NEMONAVI_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMONAVI_DEBUG_ON")
endif(WITH_NEMONAVI_DEBUG)

add_library(nemonavi SHARED
	nemonavi.cpp
	naviapp.cpp
	naviclient.cpp
	navikey.c
	)

TARGET_LINK_LIBRARIES(nemonavi
	nemoasst
	cef
	cef_dll_wrapper
	)

INSTALL(TARGETS nemonavi DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/navi/nemonavi.h
	)

set(NEMONAVI_PKGCONFIG_REQUIRES "nemoasst")

configure_file(
	nemonavi.pc.in
	nemonavi.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemonavi)
INSTALL(FILES ${CMAKE_BINARY_DIR}/navi/nemonavi.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
