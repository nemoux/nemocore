project(nemoux)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(BUILD_SHARED_LIBS ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

set(CMAKE_INSTALL_PREFIX "/usr")

set(WAYLAND_COMPOSITOR_VERSION "4" CACHE STRING "Wayland default compositor version")

option(WITH_LOGIND "Build with LOGIND" OFF)
option(WITH_XWAYLAND "Build with XWAYLAND" ON)
option(WITH_GLIB "Build with GLIB" ON)
option(WITH_PULSEAUDIO "Build with PULSEAUDIO" ON)
option(WITH_MUPDF "Build with MUPDF" ON)
option(WITH_CEF "Build with CEF" ON)
option(WITH_MINISHELL "Build with MINISHELL" ON)

option(WITH_WAYLANDSHELL "Build with WAYLANDSHELL" ON)
option(WITH_XDGSHELL "Build with XDGSHELL" ON)
option(WITH_FULLSCREEN "Build with FULLSCREEN" ON)

option(WITH_OPENGL_ES3 "Build with OPENGL ES3" ON)
option(WITH_OPENGL_PBO "Build with OPENGL PBO" OFF)
option(WITH_OPENGL_UNPACK_SUBIMAGE "Build with OPENGL's UNPACK SUBIMAGE" ON)

option(WITH_OPENGL_CS "Build with OPENGL COMPUTESHADER" OFF)

option(WITH_OPENCL "Build with OPENCL" OFF)

option(WITH_BULLET "Build with BULLET" OFF)

option(WITH_TOUCH_TO_POINTER "Build with Touch to Pointer" OFF)

option(WITH_OPTIMIZE "Build with OPTIMIZE" ON)
option(WITH_DEBUG "Build with DEBUG" OFF)
option(WITH_PROFILE "Build with PROFILE" OFF)
option(WITH_NEMO_DEBUG "Build with NEMODEBUG" ON)
option(WITH_NEMO_LOG "Build with NEMOLOG" ON)
option(WITH_XWAYLAND_DEBUG "Build with XWAYLAND DEBUG" OFF)
option(WITH_NEMONAVI_DEBUG "Build with NEMONAVI DEBUG" OFF)
option(WITH_NEMOSHOW_DEBUG "Build with NEMOSHOW DEBUG" ON)
option(WITH_NEMOPLAY_DEBUG "Build with NEMOPLAY DEBUG" OFF)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS}")

if(WITH_OPTIMIZE)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wno-unused-result")
endif(WITH_OPTIMIZE)

if(WITH_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif(WITH_DEBUG)

if(WITH_PROFILE)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
	set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -pg")
endif(WITH_PROFILE)

if(WITH_NEMO_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_DEBUG_ON")
endif(WITH_NEMO_DEBUG)

if(WITH_NEMO_LOG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMO_LOG_ON")
endif(WITH_NEMO_LOG)

if(WITH_XWAYLAND_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXWAYLAND_DEBUG_ON")
endif(WITH_XWAYLAND_DEBUG)

if(WITH_NEMONAVI_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMONAVI_DEBUG_ON")
endif(WITH_NEMONAVI_DEBUG)

if(WITH_NEMOSHOW_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMOSHOW_DEBUG_ON")
endif(WITH_NEMOSHOW_DEBUG)

if(WITH_NEMOPLAY_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNEMOPLAY_DEBUG_ON")
endif(WITH_NEMOPLAY_DEBUG)

if(WITH_OPENGL_ES3)
	add_definitions(-DNEMOUX_WITH_OPENGL_ES3=1)
endif(WITH_OPENGL_ES3)

if(WITH_OPENGL_UNPACK_SUBIMAGE)
	add_definitions(-DNEMOUX_WITH_OPENGL_UNPACK_SUBIMAGE=1)
endif(WITH_OPENGL_UNPACK_SUBIMAGE)

if(WITH_OPENGL_PBO)
	add_definitions(-DNEMOUX_WITH_OPENGL_PBO=1)
endif(WITH_OPENGL_PBO)

if(WITH_OPENGL_CS)
	add_definitions(-DNEMOUX_WITH_OPENGL_CS=1)
endif(WITH_OPENGL_CS)

if(WITH_OPENCL)
	add_definitions(-DNEMOUX_WITH_OPENCL=1)
endif(WITH_OPENCL)

if(WITH_GLIB)
	add_definitions(-DNEMOUX_WITH_GLIB=1)
endif(WITH_GLIB)

if(WITH_PULSEAUDIO)
	add_definitions(-DNEMOUX_WITH_PULSEAUDIO=1)
endif(WITH_PULSEAUDIO)

if(WITH_XWAYLAND)
	add_definitions(-DNEMOUX_WITH_XWAYLAND=1)

	add_subdirectory(xwayland)
endif(WITH_XWAYLAND)

add_subdirectory(asst)
add_subdirectory(helper)

add_subdirectory(tool)
add_subdirectory(tale)
add_subdirectory(show)
add_subdirectory(cook)
add_subdirectory(mote)
add_subdirectory(play)
add_subdirectory(pixs)
add_subdirectory(tile)
add_subdirectory(bus)
add_subdirectory(fx)
add_subdirectory(db)

if(WITH_CEF)
	add_subdirectory(navi)
endif(WITH_CEF)

add_subdirectory(compz)
add_subdirectory(shell)
add_subdirectory(sound)
add_subdirectory(envs)
add_subdirectory(apps)
add_subdirectory(utils)
