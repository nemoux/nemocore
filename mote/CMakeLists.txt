set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/asst
	${CMAKE_SOURCE_DIR}/helper
	${CMAKE_SOURCE_DIR}/mote
	)

add_library(nemomote SHARED
	nemomote.c
	nemozone.c
	builders/position.c
	builders/velocity.c
	builders/mass.c
	builders/lifetime.c
	builders/sleeptime.c
	builders/color.c
	builders/type.c
	builders/tweener.c
	emitters/random.c
	emitters/blast.c
	emitters/sine.c
	actors/move.c
	actors/accelerate.c
	actors/explosion.c
	actors/gravitywell.c
	actors/antigravity.c
	actors/mutualgravity.c
	actors/mousegravity.c
	actors/deathzone.c
	actors/livezone.c
	actors/boundingbox.c
	actors/speedscale.c
	actors/speedlimit.c
	actors/deadline.c
	actors/collide.c
	actors/tween.c
	actors/sleep.c
	)

set(NEMOMOTE_PKGCONFIG_REQUIRES "")

INSTALL(TARGETS nemomote DESTINATION lib${CMAKE_LIBRARY_SUFFIX})

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/mote/nemomote.h
	${CMAKE_SOURCE_DIR}/mote/nemozone.h
	)

set(HEADER_EMITTER_FILES
	${CMAKE_SOURCE_DIR}/mote/emitters/blast.h
	${CMAKE_SOURCE_DIR}/mote/emitters/random.h
	${CMAKE_SOURCE_DIR}/mote/emitters/sine.h
	)

set(HEADER_ACTOR_FILES
	${CMAKE_SOURCE_DIR}/mote/actors/move.h
	${CMAKE_SOURCE_DIR}/mote/actors/accelerate.h
	${CMAKE_SOURCE_DIR}/mote/actors/explosion.h
	${CMAKE_SOURCE_DIR}/mote/actors/gravitywell.h
	${CMAKE_SOURCE_DIR}/mote/actors/antigravity.h
	${CMAKE_SOURCE_DIR}/mote/actors/mutualgravity.h
	${CMAKE_SOURCE_DIR}/mote/actors/mousegravity.h
	${CMAKE_SOURCE_DIR}/mote/actors/deathzone.h
	${CMAKE_SOURCE_DIR}/mote/actors/livezone.h
	${CMAKE_SOURCE_DIR}/mote/actors/boundingbox.h
	${CMAKE_SOURCE_DIR}/mote/actors/speedscale.h
	${CMAKE_SOURCE_DIR}/mote/actors/speedlimit.h
	${CMAKE_SOURCE_DIR}/mote/actors/deadline.h
	${CMAKE_SOURCE_DIR}/mote/actors/collide.h
	${CMAKE_SOURCE_DIR}/mote/actors/tween.h
	${CMAKE_SOURCE_DIR}/mote/actors/sleep.h
	)

set(HEADER_BUILDER_FILES
	${CMAKE_SOURCE_DIR}/mote/builders/position.h
	${CMAKE_SOURCE_DIR}/mote/builders/velocity.h
	${CMAKE_SOURCE_DIR}/mote/builders/mass.h
	${CMAKE_SOURCE_DIR}/mote/builders/lifetime.h
	${CMAKE_SOURCE_DIR}/mote/builders/sleeptime.h
	${CMAKE_SOURCE_DIR}/mote/builders/color.h
	${CMAKE_SOURCE_DIR}/mote/builders/type.h
	${CMAKE_SOURCE_DIR}/mote/builders/tweener.h
	)

configure_file(
	nemomote.pc.in
	nemomote.pc
	@ONLY
	)

INSTALL(FILES ${HEADER_FILES} DESTINATION include/nemomote)
INSTALL(FILES ${HEADER_EMITTER_FILES} DESTINATION include/nemomote/emitters)
INSTALL(FILES ${HEADER_ACTOR_FILES} DESTINATION include/nemomote/actors)
INSTALL(FILES ${HEADER_BUILDER_FILES} DESTINATION include/nemomote/builders)
INSTALL(FILES ${CMAKE_BINARY_DIR}/mote/nemomote.pc DESTINATION lib${CMAKE_LIBRARY_SUFFIX}/pkgconfig)
