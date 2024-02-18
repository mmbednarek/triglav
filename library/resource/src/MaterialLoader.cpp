#include "MaterialLoader.h"

#include "triglav/render_core/Material.hpp"

namespace triglav::resource {

render_core::Material Loader<ResourceType::Material>::load(std::string_view path)
{
   using namespace name_literals;
   return render_core::Material{
           .texture        = "default.tex"_name,
           .normal_texture = "default/normal.tex"_name,
           .props          = render_core::MaterialProps{false, 1.0f, 0.0f}
   };
}

}// namespace triglav::resource
