#pragma once

#include "triglav/graphics_api/CommandList.h"

namespace triglav::graphics_api {

void copy_texture(graphics_api::CommandList& cmdList, const graphics_api::Texture& src, const graphics_api::Texture& dst);

}