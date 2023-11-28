set(RENDERER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(renderer STATIC)

# Public include headers
target_include_directories(renderer
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/renderer
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(renderer
        LINK_PUBLIC
        graphics_api
        object_reader
        glm
)
