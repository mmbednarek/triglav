set(OBJECT_READER_INCLUDE_DIR ${OBJECT_READER_DIR}/include/geometry)

target_sources(geometry
    # Source files
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/BufferedReader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InternalMesh.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InternalMesh.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp

    # Header files
    PUBLIC
        ${OBJECT_READER_INCLUDE_DIR}/BufferedReader.h
        ${OBJECT_READER_INCLUDE_DIR}/Geometry.h
        ${OBJECT_READER_INCLUDE_DIR}/Mesh.h
        ${OBJECT_READER_INCLUDE_DIR}/Parser.h
)
