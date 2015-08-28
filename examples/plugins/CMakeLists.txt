include_directories(
	${CMAKE_SOURCE_DIR}/asst
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_library(miniplugin SHARED
	miniplugin.c
	)

TARGET_LINK_LIBRARIES(miniplugin
	nemoasst
	)
