FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(JSON REQUIRED json-c)
INCLUDE_DIRECTORIES(${JSON_INCLUDE_DIRS})
LINK_DIRECTORIES(${JSON_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemolog
	nemolog.c
	)

TARGET_LINK_LIBRARIES(nemolog
	nemoasst
	)

INSTALL(TARGETS nemolog DESTINATION bin)

add_executable(nemokeys
	nemokeys.c
	)

TARGET_LINK_LIBRARIES(nemokeys
	nemoasst
	)

INSTALL(TARGETS nemokeys DESTINATION bin)

add_executable(nemonote
	nemonote.c
	)

TARGET_LINK_LIBRARIES(nemonote
	nemoasst
	${JSON_LIBRARIES}
	)

INSTALL(TARGETS nemonote DESTINATION bin)
