cmake_minimum_required(VERSION 3.9)
project(vulkan_game)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan)
find_package(Fmt)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL)
find_package(glm)

find_library(WAYLAND_CLIENT_LIBRARY_PATH NAMES wayland-client)
if (WAYLAND_CLIENT_LIBRARY_PATH)
	message(STATUS "Found wayland client: ${WAYLAND_CLIENT_LIBRARY_PATH}")
	add_library(Wayland::Client SHARED IMPORTED)
	set_property(TARGET Wayland::Client PROPERTY IMPORTED_LOCATION ${WAYLAND_CLIENT_LIBRARY_PATH})
else ()
	message(FATAL_ERROR "Failed to find wayland-client library")
endif ()

find_library(WAYLAND_EGL_LIBRARY_PATH NAMES wayland-egl)
if (WAYLAND_EGL_LIBRARY_PATH)
	message(STATUS "Found wayland EGL: ${WAYLAND_EGL_LIBRARY_PATH}")
	add_library(Wayland::EGL SHARED IMPORTED)
	set_property(TARGET Wayland::EGL PROPERTY IMPORTED_LOCATION ${WAYLAND_EGL_LIBRARY_PATH})
else ()
	message(FATAL_ERROR "Failed to find wayland-egl library")
endif ()

add_subdirectory(library/graphics_api)
add_subdirectory(library/renderer)

if (WIN32)
	add_subdirectory(game/win32)
elseif(UNIX)
	add_subdirectory(game/linux_wayland)
endif()
