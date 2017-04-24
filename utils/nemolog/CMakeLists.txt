include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/kits
	${CMAKE_SOURCE_DIR}/helper
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemolog
	nemolog.c
	)

TARGET_LINK_LIBRARIES(nemolog
	nemoasst
	nemokits
	nemohelper
	)

INSTALL(TARGETS nemolog DESTINATION bin)
