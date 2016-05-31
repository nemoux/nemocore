FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

PKG_CHECK_MODULES(AO REQUIRED ao)
INCLUDE_DIRECTORIES(${AO_INCLUDE_DIRS})
LINK_DIRECTORIES(${AO_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/apps
	${CMAKE_SOURCE_DIR}/msg
	${CMAKE_SOURCE_DIR}/play
	${CMAKE_SOURCE_DIR}/navi
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemoimage-show
	nemoimage-show.c
	)

TARGET_LINK_LIBRARIES(nemoimage-show
	nemoapps
	)

add_executable(nemoplay-decode
	nemoplay-decode.c
	)

TARGET_LINK_LIBRARIES(nemoplay-decode
	nemoplay
	${AO_LIBRARIES}
	)

add_executable(nemoplay-show
	nemoplay-show.c
	)

TARGET_LINK_LIBRARIES(nemoplay-show
	nemoplay
	nemoapps
	${AO_LIBRARIES}
	)

if(WITH_CEF)
	add_executable(nemonavi-show
		nemonavi-show.c
		)

	TARGET_LINK_LIBRARIES(nemonavi-show
		nemonavi
		nemoapps
		)
endif(WITH_CEF)
