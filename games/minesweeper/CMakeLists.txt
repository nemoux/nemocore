FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_definitions(-DNEMOUX_MINESWEEPER_ICONS="${CMAKE_INSTALL_PREFIX}/share/nemogames/minesweeper")

add_executable(nemomine
	nemomine.cpp
	showhelper.c
	)

TARGET_LINK_LIBRARIES(nemomine
	nemoasst
	nemotool
	nemotale
	nemoshow
	)

set(ICON_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-0.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-1.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-2.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-3.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-4.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-5.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-6.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-7.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-8.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-9.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-bomb.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-finder.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-help.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-pin.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-quit.svg
	${CMAKE_CURRENT_SOURCE_DIR}/icons/mine-square.svg
	)

INSTALL(TARGETS nemomine DESTINATION bin)
INSTALL(FILES ${ICON_FILES} DESTINATION share/nemogames/minesweeper)
