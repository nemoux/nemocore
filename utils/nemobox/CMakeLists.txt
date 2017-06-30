FIND_PACKAGE(PkgConfig REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

add_executable(nemobox
	nemobox.cpp
	)

qt5_use_modules(nemobox
	WebEngineWidgets
	)

INSTALL(TARGETS nemobox DESTINATION bin)
