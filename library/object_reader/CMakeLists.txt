set(OBJECT_READER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_library(object_reader STATIC)

# Public include headers
target_include_directories(object_reader
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/object_reader
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(object_reader
        LINK_PUBLIC
        glm
)
