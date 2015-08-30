set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/common
	${CMAKE_CURRENT_SOURCE_DIR}/sweep
	)

add_library(poly2tri SHARED
	common/shapes.cc
	sweep/cdt.cc
	sweep/sweep.cc
	sweep/advancing_front.cc
	sweep/sweep_context.cc
	)

INSTALL(TARGETS poly2tri DESTINATION lib${CMAKE_LIBRARY_SUFFIX})
