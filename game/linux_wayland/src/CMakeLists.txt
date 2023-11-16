target_sources(linux_wayland
        # Source files
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/api/PointerConstraints.c
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/api/PointerConstraints.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/api/RelativePointer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/api/RelativePointer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/api/XdgShell.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/api/XdgShell.c
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Display.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Display.h
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/IDisplayEventListener.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Surface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland/Surface.h
)
