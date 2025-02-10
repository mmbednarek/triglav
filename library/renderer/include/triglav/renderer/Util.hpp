#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer {

void blur_texture(render_core::BuildContext& ctx, Name srcTexture, Name dstTexture, graphics_api::ColorFormat dstFormat);

}