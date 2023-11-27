set(GRAPHICS_API_INCLUDE_DIR ${GRAPHICS_API_DIR}/include/graphics_api)

target_sources(graphics_api
    # Source files
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Buffer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/CommandList.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DescriptorGroup.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PipelineBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PlatformSurface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/RenderPass.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Synchronization.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Texture.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/Extensions.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/Util.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/Util.h

    # Header files
    PUBLIC
        ${GRAPHICS_API_INCLUDE_DIR}/Array.hpp
        ${GRAPHICS_API_INCLUDE_DIR}/Buffer.h
        ${GRAPHICS_API_INCLUDE_DIR}/CommandList.h
        ${GRAPHICS_API_INCLUDE_DIR}/GraphicsApi.hpp
        ${GRAPHICS_API_INCLUDE_DIR}/Device.h
        ${GRAPHICS_API_INCLUDE_DIR}/DescriptorGroup.h
        ${GRAPHICS_API_INCLUDE_DIR}/Pipeline.h
        ${GRAPHICS_API_INCLUDE_DIR}/PipelineBuilder.h
        ${GRAPHICS_API_INCLUDE_DIR}/PlatformSurface.h
        ${GRAPHICS_API_INCLUDE_DIR}/RenderPass.h
        ${GRAPHICS_API_INCLUDE_DIR}/Shader.h
        ${GRAPHICS_API_INCLUDE_DIR}/Synchronization.h
        ${GRAPHICS_API_INCLUDE_DIR}/Texture.h
        ${GRAPHICS_API_INCLUDE_DIR}/vulkan/Extensions.h
        ${GRAPHICS_API_INCLUDE_DIR}/vulkan/ObjectWrapper.hpp
)

