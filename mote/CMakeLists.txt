set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

include_directories(
	${CMAKE_SOURCE_DIR}/mote
	)

add_library(nemomote SHARED
	nemomote.c
	nemozone.c
	builders/position.c
	builders/velocity.c
	builders/mass.c
	builders/color.c
	emitters/random.c
	emitters/blast.c
	emitters/sine.c
	actors/move.c
	actors/accelerate.c
	actors/gravitywall.c
	actors/mutualgravity.c
	actors/mousegravity.c
	actors/deathzone.c
	actors/livezone.c
	actors/boundingbox.c
	actors/speedscale.c
	)

INSTALL(TARGETS nemomote DESTINATION lib${CMAKE_LIBRARY_SUFFIX})
