set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/compz
	${CMAKE_BINARY_DIR}/compz
	${CMAKE_SOURCE_DIR}/shell
	${CMAKE_BINARY_DIR}/shell
	/usr/include/skia/core
	/usr/include/skia/config
	/usr/include/skia/animator
	/usr/include/skia/codec
	/usr/include/skia/device
	/usr/include/skia/effects
	/usr/include/skia/images
	/usr/include/skia/pathops
	/usr/include/skia/pipe
	/usr/include/skia/ports
	/usr/include/skia/svg
	/usr/include/skia/utils
	/usr/include/skia/views
	/usr/include/skia/xml
	)

if(WITH_XWAYLAND)
	include_directories(
		${CMAKE_SOURCE_DIR}/xwayland
		)
endif(WITH_XWAYLAND)

link_directories(
	/usr/lib64/skia
	)

set(CMAKE_INSTALL_RPATH "/usr/lib64/skia")

add_executable(minishell
	minishell.cpp
	minigrab.cpp
	miniyoyo.cpp
	minipalm.cpp
	talehelper.c
	showhelper.c
	)

TARGET_LINK_LIBRARIES(minishell
	nemotale
	nemoshow
	nemocompz
	nemoshell
	${NEMOUX_COMMON_LIBRARIES}
	)

if(WITH_XWAYLAND)
	TARGET_LINK_LIBRARIES(minishell
		nemoxwayland
		)
endif(WITH_XWAYLAND)
