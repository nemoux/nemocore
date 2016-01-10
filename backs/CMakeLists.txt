FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/mote
	${CMAKE_SOURCE_DIR}/mosi
	${CMAKE_SOURCE_DIR}/apps
	${CMAKE_SOURCE_DIR}/thirds
	${CMAKE_SOURCE_DIR}/thirds/poly2tri
	)

add_executable(nemoback-mini
	miniback.c
	)

TARGET_LINK_LIBRARIES(nemoback-mini
	nemoapps
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-mini DESTINATION bin)

add_executable(nemoback-miro
	miroback.c
	)

TARGET_LINK_LIBRARIES(nemoback-miro
	nemoapps
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-miro DESTINATION bin)

add_executable(nemoback-plex
	plexback.cpp
	)

TARGET_LINK_LIBRARIES(nemoback-plex
	nemoapps
	poly2tri
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-plex DESTINATION bin)

add_executable(nemoback-mote
	moteback.cpp
	)

TARGET_LINK_LIBRARIES(nemoback-mote
	nemoapps
	nemomote
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-mote DESTINATION bin)

add_executable(nemoback-mosi
	mosiback.cpp
	)

TARGET_LINK_LIBRARIES(nemoback-mosi
	nemoapps
	nemomosi
	${NEMOUX_COMMON_LIBRARIES}
	${SKIA_LIBRARIES}
	)

INSTALL(TARGETS nemoback-mosi DESTINATION bin)
