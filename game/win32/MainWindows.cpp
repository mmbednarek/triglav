#include "GraphicsDevice.h"

#include <algorithm>
#include <fstream>

#include <Windows.h>

constexpr auto WndClassName = L"VulkanTestCls";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

std::vector<uint8_t> read_whole_file(std::string_view name)
{
    std::ifstream file(std::string{name}, std::ios::ate | std::ios::binary);
    if (not file.is_open())
    {
        return {};
    }

    file.seekg(0, std::ios::end);
    const auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> result{};
    result.resize(fileSize);

    file.read(reinterpret_cast<char*>(result.data()), fileSize);
    return result;
}

class Renderer
{
public:
    bool init(HWND hwnd, HINSTANCE hInstance)
    {
        const graphics_api::Surface surface{
            .windowHandle = hwnd,
            .processInstance = hInstance,
        };
        
        m_device = graphics_api::initialize_device(surface).value_or(nullptr);
        if (m_device == nullptr)
        {
            return false;
        }

        RECT windowRect{};
        if (not GetWindowRect(hwnd, &windowRect))
        {
            return false;
        }

        graphics_api::Resolution resolution{
            .width = static_cast<uint32_t>(windowRect.right - windowRect.left),
            .height = static_cast<uint32_t>(windowRect.bottom - windowRect.top)
        };

        const auto [minResolution, maxResolution] = m_device->get_surface_resolution_limits();
        resolution.width = std::clamp(resolution.width, minResolution.width, maxResolution.width);
        resolution.height = std::clamp(resolution.height, minResolution.height, maxResolution.height);

        if (m_device->init_swapchain(GAPI_COLOR_FORMAT(BGRA, sRGB), graphics_api::ColorSpace::sRGB, resolution) != graphics_api::Status::Success)
        {
            return false;
        }

        const auto vertexShaderData = read_whole_file("../shader/example_vertex.spv");
        if (vertexShaderData.empty())
        {
            return false;
        }

        auto vertexShader = m_device->create_shader(graphics_api::ShaderStage::Vertex, "main", vertexShaderData);
        if (not vertexShader.has_value())
        {
            return false;
        }
        m_vertexShader = std::move(*vertexShader);

        const auto fragmentShaderData = read_whole_file("../shader/example_fragment.spv");
        if (fragmentShaderData.empty())
        {
            return false;
        }

        auto fragmentShader = m_device->create_shader(graphics_api::ShaderStage::Fragment, "main", fragmentShaderData);
        if (not fragmentShader.has_value())
        {
            return false;
        }
        m_fragmentShader = std::move(*fragmentShader);

        std::array shaders{
            &(*m_vertexShader),
            &(*m_fragmentShader),
        };

        auto pipeline = m_device->create_pipeline(shaders);
        if (not pipeline.has_value())
        {
            return false;
        }
        m_pipeline = std::move(*pipeline);

        auto waitSemaphore = m_device->create_semaphore();
        if (not waitSemaphore.has_value())
        {
            return false;
        }
        m_framebufferReadySemaphore = std::move(*waitSemaphore);

        auto signalSemaphore = m_device->create_semaphore();
        if (not signalSemaphore.has_value())
        {
            return false;
        }
        m_renderFinishedSemaphore = std::move(*signalSemaphore);

        auto fence = m_device->create_fence();
        if (not fence.has_value())
        {
            return false;
        }
        m_inFlightFence = std::move(*fence);

        auto commandList = m_device->create_command_list();
        if (not commandList.has_value())
        {
            return false;
        }
        m_commandList = std::move(*commandList);

        return true;
    }

    bool render() const
    {
        const auto framebufferIndex = m_device->get_available_framebuffer(*m_framebufferReadySemaphore);

        m_inFlightFence->await();

        if (m_device->init_command_list(*m_commandList, framebufferIndex, graphics_api::ColorPalette::Red) != graphics_api::Status::Success)
        {
            return false;
        }

        m_commandList->bind_pipeline(*m_pipeline);
        m_commandList->draw_primitives(3, 0);

        if (m_commandList->finish() != graphics_api::Status::Success)
        {
            return false;
        }

        if (m_device->submit_command_list(*m_commandList, *m_framebufferReadySemaphore, *m_renderFinishedSemaphore, *m_inFlightFence) != graphics_api::Status::Success)
        {
            return false;
        }

        if (m_device->present(*m_renderFinishedSemaphore, framebufferIndex) != graphics_api::Status::Success)
        {
           return false;
        }

        return true;
    }

    void on_close() const
    {
        m_inFlightFence->await();
    }

private:
    graphics_api::DeviceUPtr m_device;
    std::optional<graphics_api::Shader> m_vertexShader;
    std::optional<graphics_api::Shader> m_fragmentShader;
    std::optional<graphics_api::Pipeline> m_pipeline;
    std::optional<graphics_api::Semaphore> m_framebufferReadySemaphore;
    std::optional<graphics_api::Semaphore> m_renderFinishedSemaphore;
    std::optional<graphics_api::Fence> m_inFlightFence;
    std::optional<graphics_api::CommandList> m_commandList;
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = hInstance;
    wc.lpszClassName = WndClassName;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowExW(
        0, // Optional window styles.
        WndClassName, // Window class
        L"Learn to Program Windows", // Window text
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        nullptr, // Parent window    
        nullptr, // Menu
        hInstance, // Instance handle
        nullptr // Additional application data
    );

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    Renderer renderer;

    if(not renderer.init(hwnd, hInstance))
    {
        return 1;
    }


    // Run the message loop.
    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        if (not renderer.render())
        {
            PostQuitMessage(1);
        }
    }

    renderer.on_close();
    DestroyWindow(hwnd);
    return 0;
}

LRESULT CALLBACK WindowProc(const HWND hwnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
      case WM_PAINT:
        ValidateRect(hwnd, nullptr);
        break;
    default: break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
