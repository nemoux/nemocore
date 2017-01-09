FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(PIXMAN REQUIRED pixman-1)
INCLUDE_DIRECTORIES(${PIXMAN_INCLUDE_DIRS})
LINK_DIRECTORIES(${PIXMAN_LIBRARY_DIRS})

PKG_CHECK_MODULES(JSON REQUIRED json-c)
INCLUDE_DIRECTORIES(${JSON_INCLUDE_DIRS})
LINK_DIRECTORIES(${JSON_LIBRARY_DIRS})

option(WITH_TCMALLOC "Build with TCMALLOC" ON)
option(WITH_MINISHELL "Build with MINISHELL" ON)

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/helper
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/bus
	${CMAKE_SOURCE_DIR}/db
	${CMAKE_SOURCE_DIR}/play
	${CMAKE_SOURCE_DIR}/cook
	${CMAKE_SOURCE_DIR}/action
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemolog
	nemolog.c
	)

TARGET_LINK_LIBRARIES(nemolog
	nemoasst
	nemohelper
	)

INSTALL(TARGETS nemolog DESTINATION bin)

add_executable(nemobusc
	nemobusc.c
	)

TARGET_LINK_LIBRARIES(nemobusc
	nemoasst
	nemobus
	${JSON_LIBRARIES}
	)

INSTALL(TARGETS nemobusc DESTINATION bin)

add_executable(nemobusd
	nemobusd.c
	)

TARGET_LINK_LIBRARIES(nemobusd
	nemoasst
	nemobus
	${JSON_LIBRARIES}
	)

INSTALL(TARGETS nemobusd DESTINATION bin)

add_executable(nemodbs
	nemodbs.c
	)

TARGET_LINK_LIBRARIES(nemodbs
	nemoasst
	nemodb
	)

INSTALL(TARGETS nemodbs DESTINATION bin)

add_executable(nemoart
	nemoart.c
	)

TARGET_LINK_LIBRARIES(nemoart
	nemoasst
	nemotool
	nemoplay
	nemocook
	nemoaction
	nemobus
	)

if(WITH_TCMALLOC)
	TARGET_LINK_LIBRARIES(nemoart
		tcmalloc_minimal
		)
endif(WITH_TCMALLOC)

INSTALL(TARGETS nemoart DESTINATION bin)

if(WITH_MINISHELL)
	include_directories(
		${CMAKE_SOURCE_DIR}/compz
		${CMAKE_SOURCE_DIR}/shell
		${CMAKE_SOURCE_DIR}/xwayland
		${CMAKE_SOURCE_DIR}/envs
		)

	add_executable(minishell
		minishell.c
		)

	TARGET_LINK_LIBRARIES(minishell
		nemoasst
		nemohelper
		nemocompz
		nemoshell
		nemoxwayland
		nemoenvs
		nemoplay
		nemocook
		nemobus
		nemodb
		)

	if(WITH_TCMALLOC)
		TARGET_LINK_LIBRARIES(minishell
			tcmalloc_minimal
			)
	endif(WITH_TCMALLOC)

	INSTALL(TARGETS minishell DESTINATION bin)
endif(WITH_MINISHELL)
