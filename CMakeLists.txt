project(nemoux)

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(BUILD_SHARED_LIBS ON)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

set(CMAKE_INSTALL_PREFIX "/usr")
set(CMAKE_INSTALL_PACKAGES "/opt/pkgs")

add_definitions(-DNEMOUX_INSTALL_PACKAGES="${CMAKE_INSTALL_PACKAGES}")

option(WITH_OPTIMIZE "Build with OPTIMIZE" ON)
option(WITH_DEBUG "Build with DEBUG" OFF)
option(WITH_PROFILE "Build with PROFILE" OFF)
option(WITH_NEMO_DEBUG "Build with NEMODEBUG" ON)
option(WITH_NEMO_LOG "Build with NEMOLOG" ON)

option(WITH_QT5_WEBENGINE "Build with QT5WEBENGINE" OFF)
option(WITH_SKIA "Build with SKIA" OFF)

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

add_subdirectory(asst)
add_subdirectory(kits)
add_subdirectory(helper)

add_subdirectory(tool)
add_subdirectory(cook)
add_subdirectory(action)
add_subdirectory(play)

add_subdirectory(xwayland)

add_subdirectory(compz)
add_subdirectory(shell)

add_subdirectory(envs)

add_subdirectory(utils/nemolog)
add_subdirectory(utils/nemoart)
add_subdirectory(utils/nemobusd)
add_subdirectory(utils/nemosound)
add_subdirectory(utils/minishell)

if(WITH_QT5_WEBENGINE)
	add_subdirectory(utils/nemobox)
endif(WITH_QT5_WEBENGINE)

if(WITH_SKIA)
	add_subdirectory(tozz)
	add_subdirectory(motz)
endif(WITH_SKIA)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.json DESTINATION ${CMAKE_INSTALL_PACKAGES}/nemocore/)
