set(OBJECT_READER_INCLUDE_DIR ${OBJECT_READER_DIR}/include/object_reader)

target_sources(object_reader
    # Source files
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/BufferedReader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Reader.cpp

    # Header files
    PUBLIC
        ${OBJECT_READER_INCLUDE_DIR}/BufferedReader.h
        ${OBJECT_READER_INCLUDE_DIR}/Parser.h
        ${OBJECT_READER_INCLUDE_DIR}/Reader.h
)
