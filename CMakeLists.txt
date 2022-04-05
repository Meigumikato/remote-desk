cmake_minimum_required(VERSION 3.20)


project(
	remote-desk
	VERSION 0.1
	LANGUAGES CXX
)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS -fsanitize=address -g")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
	libavdevice
	libavfilter
	libavformat
	libavcodec
	libswresample
	libswscale
	libavutil
	)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_link_libraries(${PROJECT_NAME}
	PRIVATE
	PkgConfig::LIBAV
	SDL2
)

add_executable(${PROJECT_NAME}_player)
target_sources(${PROJECT_NAME}_player PRIVATE player.cpp)
target_link_libraries(${PROJECT_NAME}_player
	PRIVATE
	PkgConfig::LIBAV
	SDL2
)

add_executable(${PROJECT_NAME}-new)
target_sources(${PROJECT_NAME}-new PRIVATE try.cpp)
target_link_libraries(${PROJECT_NAME}-new
	PRIVATE
	PkgConfig::LIBAV
	SDL2
)

add_executable(${PROJECT_NAME}-player)
target_sources(${PROJECT_NAME}-player PRIVATE play/player.cpp)
target_link_libraries(${PROJECT_NAME}-player
	PRIVATE
	PkgConfig::LIBAV
	SDL2
)
