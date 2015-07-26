set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/show
	)

add_executable(show-simple
	show-simple.c
	)

TARGET_LINK_LIBRARIES(show-simple
	nemoshow
	${NEMOUX_COMMON_LIBRARIES}
	)
