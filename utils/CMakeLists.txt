FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(JSON REQUIRED json-c)
INCLUDE_DIRECTORIES(${JSON_INCLUDE_DIRS})
LINK_DIRECTORIES(${JSON_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/bus
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
