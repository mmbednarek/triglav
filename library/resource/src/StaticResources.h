#pragma once

namespace triglav::graphics_api {
class Device;
}

namespace triglav::resource {

class ResourceManager;

void register_samplers(graphics_api::Device& device, ResourceManager& manager);

}// namespace triglav::resource