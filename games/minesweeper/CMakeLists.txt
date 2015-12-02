FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/apps
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_definitions(-DNEMOUX_MINESWEEPER_RESOURCES="${CMAKE_INSTALL_PREFIX}/share/nemogames/minesweeper")

add_executable(nemomine
	nemomine.cpp
	)

TARGET_LINK_LIBRARIES(nemomine
	nemoapps
	)

set(RESOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-0.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-1.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-2.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-3.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-4.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-5.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-6.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-7.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-8.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-9.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-bomb.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-finder.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-help.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-pin.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-quit.svg
	${CMAKE_CURRENT_SOURCE_DIR}/resources/mine-square.svg
	)

INSTALL(TARGETS nemomine DESTINATION bin)
INSTALL(FILES ${RESOURCE_FILES} DESTINATION share/nemogames/minesweeper)
