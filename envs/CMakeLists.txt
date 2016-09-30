FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SKIA REQUIRED skia)
INCLUDE_DIRECTORIES(${SKIA_INCLUDE_DIRS})
LINK_DIRECTORIES(${SKIA_LIBRARY_DIRS})

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/compz
	${CMAKE_SOURCE_DIR}/shell
	${CMAKE_BINARY_DIR}/shell
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/xwayland
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_definitions(-DNEMOENVS_RESOURCES="${CMAKE_INSTALL_PREFIX}/share/nemoenvs")

add_library(nemoenvs SHARED
	nemoenvs.c
	nemoapps.c
	nemoxapp.c
	bindings.c
	configs.c
	showhelper.c
	)

TARGET_LINK_LIBRARIES(nemoenvs
	nemoasst
	nemocompz
	nemoshell
	nemotale
	nemoshow
	nemoxwayland
	)

set(NEMOENVS_PKGCONFIG_REQUIRES "nemoasst nemocompz nemoshell nemoshow nemoxwayland")

INSTALL(TARGETS nemoenvs DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	nemoenvs.h
	nemoapps.h
	nemoxapp.h
	showhelper.h
	)

configure_file(
	nemoenvs.pc.in
	nemoenvs.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemoenvs)
INSTALL(FILES ${CMAKE_BINARY_DIR}/envs/nemoenvs.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
