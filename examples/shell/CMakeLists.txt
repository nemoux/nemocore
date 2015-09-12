set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
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
	)

if(WITH_XWAYLAND)
	include_directories(
		${CMAKE_SOURCE_DIR}/xwayland
		)
endif(WITH_XWAYLAND)

add_executable(minishell
	minishell.cpp
	minigrab.cpp
	miniyoyo.cpp
	minipalm.cpp
	talehelper.c
	showhelper.c
	skiahelper.cpp
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
