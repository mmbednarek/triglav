set(OBJECT_READER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(geometry STATIC)

# Public include headers
target_include_directories(geometry
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/geometry
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(geometry
        LINK_PUBLIC
        glm
        CGAL::CGAL
)
