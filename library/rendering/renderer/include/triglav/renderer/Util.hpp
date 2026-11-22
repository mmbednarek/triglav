#pragma once

#include "triglav/Name.hpp"
#include "triglav/render_core/Format.hpp"

namespace triglav::render_core {
class BuildContext;
}

namespace triglav::renderer {

void blur_texture(render_core::BuildContext& ctx, Name src_texture, Name dst_texture, render_core::Format dst_format);

}