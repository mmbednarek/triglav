#include "SplashScreen.h"

#include <utility>

namespace demo {

using namespace triglav::name_literals;

using triglav::graphics_api::AttachmentAttribute;
using triglav::graphics_api::ColorFormat;
using triglav::graphics_api::ColorSpace;
using triglav::graphics_api::RenderTargetBuilder;
using triglav::graphics_api::Resolution;

constexpr Resolution g_splashScreenResolution{1024, 360};
constexpr ColorFormat g_splashScreenColorFormat{GAPI_FORMAT(BGRA, sRGB)};

SplashScreen::SplashScreen(triglav::graphics_api::Surface& surface, triglav::graphics_api::Device& device) :
    m_surface(surface),
    m_device(device),
    m_swapchain(GAPI_CHECK(m_device.create_swapchain(m_surface, GAPI_FORMAT(BGRA, sRGB), ColorSpace::sRGB, g_splashScreenResolution))),
    m_renderTarget(GAPI_CHECK(RenderTargetBuilder(m_device)
                                 .attachment("output"_name,
                                             AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                AttachmentAttribute::StoreImage | AttachmentAttribute::Presentable,
                                             g_splashScreenColorFormat)
                                 .build()))
{
}

}// namespace demo
