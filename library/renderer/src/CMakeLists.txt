set(RENDERER_INCLUDE_DIR ${RENDERER_DIR}/include/renderer)

target_sources(renderer
    # Source files
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/DebugMeshes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp

    # Header files
    PUBLIC
        ${RENDERER_INCLUDE_DIR}/Core.h
        ${RENDERER_INCLUDE_DIR}/DebugMeshes.h
        ${RENDERER_INCLUDE_DIR}/Renderer.h
)
