FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/play
	${CMAKE_SOURCE_DIR}/apps
	${CMAKE_SOURCE_DIR}/fx
	${CMAKE_SOURCE_DIR}/thirds
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemotile
	nemotile.c
	tilehelper.cpp
	${CMAKE_SOURCE_DIR}/misc/glhelper.c
	${CMAKE_SOURCE_DIR}/thirds/tiny_obj_loader.cc
	)

TARGET_LINK_LIBRARIES(nemotile
	nemoapps
	nemofx
	nemoplay
	)
