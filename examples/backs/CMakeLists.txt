set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/show
	${CMAKE_SOURCE_DIR}/tale
	${CMAKE_SOURCE_DIR}/tool
	${CMAKE_BINARY_DIR}/tool
	${CMAKE_SOURCE_DIR}/thirds/poly2tri
	/usr/include/skia/core
	/usr/include/skia/config
	/usr/include/skia/animator
	/usr/include/skia/codec
	/usr/include/skia/device
	/usr/include/skia/effects
	/usr/include/skia/images
	/usr/include/skia/pathops
	/usr/include/skia/pipe
	/usr/include/skia/ports
	/usr/include/skia/svg
	/usr/include/skia/utils
	/usr/include/skia/views
	/usr/include/skia/xml
	)

link_directories(
	/usr/lib64/skia
	)

set(CMAKE_INSTALL_RPATH "/usr/lib64/skia")

add_executable(miniback
	miniback.c
	talehelper.c
	)

TARGET_LINK_LIBRARIES(miniback
	nemotale
	nemotool
	${NEMOUX_COMMON_LIBRARIES}
	)

add_executable(plexback
	plexback.cpp
	talehelper.c
	)

TARGET_LINK_LIBRARIES(plexback
	nemotool
	nemotale
	skia
	poly2tri
	${NEMOUX_COMMON_LIBRARIES}
	)
