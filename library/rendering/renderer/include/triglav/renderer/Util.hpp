#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer {

void blur_texture(render_core::BuildContext& ctx, Name src_texture, Name dst_texture, graphics_api::ColorFormat dst_format);

}