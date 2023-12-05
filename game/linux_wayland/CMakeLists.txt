set(LINUX_WAYLAND_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(linux_wayland)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(linux_wayland
        LINK_PRIVATE
        renderer
        geometry
        Wayland::Client
        fmt::fmt
        xkbcommon
)

