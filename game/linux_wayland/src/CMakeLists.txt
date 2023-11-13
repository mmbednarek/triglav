target_sources(linux_wayland
        # Source files
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/xdg-shell-protocol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/xdg-shell-client-protocol.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Display.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Display.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Surface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Surface.h
)
