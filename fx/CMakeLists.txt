FIND_PACKAGE(PkgConfig REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/misc
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/thirds
	)

add_library(nemofx SHARED
	glfilter.c
	glripple.c
	glblur.c
	glmask.c
	glmotion.c
	gllight.c
	glshadow.c
	glswirl.c
	glsweep.c
	glpolar.c
	glcolor.c
	fxnoise.cpp
	fxsph.c
	${CMAKE_SOURCE_DIR}/thirds/fastnoise.cpp
	)

TARGET_LINK_LIBRARIES(nemofx
	nemoasst
	)

INSTALL(TARGETS nemofx DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	glfilter.h
	glripple.h
	glblur.h
	glmask.h
	glmotion.h
	gllight.h
	glshadow.h
	glswirl.h
	glsweep.h
	glpolar.h
	glcolor.h
	fxnoise.h
	fxsph.h
	)

set(NEMOPLAY_PKGCONFIG_REQUIRES "nemoasst")

configure_file(
	nemofx.pc.in
	nemofx.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemofx)
INSTALL(FILES ${CMAKE_BINARY_DIR}/fx/nemofx.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
