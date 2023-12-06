set(RENDERER_INCLUDE_DIR ${RENDERER_DIR}/include/renderer)

target_sources(renderer
    # Source files
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SkyBox.cpp

    # Header files
    PUBLIC
        ${RENDERER_INCLUDE_DIR}/Core.h
        ${RENDERER_INCLUDE_DIR}/Renderer.h
        ${RENDERER_INCLUDE_DIR}/SkyBox.h
)
