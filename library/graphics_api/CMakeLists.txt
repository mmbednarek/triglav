set(GRAPHICS_API_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(graphics_api STATIC)

# Public include headers
target_include_directories(graphics_api 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/graphics_api
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(graphics_api
        LINK_PRIVATE
        Vulkan::Vulkan
        Wayland::Client
)
