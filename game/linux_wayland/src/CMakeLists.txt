target_sources(linux_wayland
        # Source files
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell-protocol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell-client-protocol.h
)
