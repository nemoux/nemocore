include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tale
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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON -DNEMO_DEBUG_ON")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(minishell
	minishell.c
	talehelper.c
	)

TARGET_LINK_LIBRARIES(minishell
	nemotale
	nemocompz
	nemoshell
	${NEMOUX_COMMON_LIBRARIES}
	)

if(WITH_XWAYLAND)
	TARGET_LINK_LIBRARIES(minishell
		nemoxwayland
		)
endif(WITH_XWAYLAND)

