set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	)

link_directories(
	/usr/lib64/skia
	)

set(CMAKE_INSTALL_RPATH "/usr/lib64/skia")

add_executable(show-simple
	show-simple.c
	)

TARGET_LINK_LIBRARIES(show-simple
	nemoshow
	${NEMOUX_COMMON_LIBRARIES}
	)

add_executable(show-tale
	show-tale.c
	showhelper.c
	)

TARGET_LINK_LIBRARIES(show-tale
	nemoshow
	nemotale
	nemotool
	${NEMOUX_COMMON_LIBRARIES}
	)
