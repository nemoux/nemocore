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
	${CMAKE_SOURCE_DIR}/apps
	${CMAKE_SOURCE_DIR}/play
	${CMAKE_SOURCE_DIR}/navi
	${CMAKE_SOURCE_DIR}/fx
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemoplay-show
	nemoplay-show.c
	)

TARGET_LINK_LIBRARIES(nemoplay-show
	nemoplay
	nemoapps
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

add_executable(nemomote-show
	nemomote-show.c
	)

TARGET_LINK_LIBRARIES(nemomote-show
	nemomote
	nemoapps
	)

if(WITH_OPENCL)
	PKG_CHECK_MODULES(OPENCL REQUIRED OpenCL)
	INCLUDE_DIRECTORIES(${OPENCL_INCLUDE_DIRS})
	LINK_DIRECTORIES(${OPENCL_LIBRARY_DIRS})

	TARGET_LINK_LIBRARIES(nemomote-show
		${OPENCL_LIBRARIES}
		)
endif(WITH_OPENCL)

if(WITH_BULLET)
	PKG_CHECK_MODULES(BULLET REQUIRED bullet)
	INCLUDE_DIRECTORIES(${BULLET_INCLUDE_DIRS})
	LINK_DIRECTORIES(${BULLET_LIBRARY_DIRS})

	add_executable(nemophys-2d
		nemophys-2d.cpp
		)

	TARGET_LINK_LIBRARIES(nemophys-2d
		nemoapps
		${BULLET_LIBRARIES}
		)
endif(WITH_BULLET)

add_executable(nemofx-show
	nemofx-show.c
	)

TARGET_LINK_LIBRARIES(nemofx-show
	nemoapps
	nemofx
	)
