#include "ComponentManager.hpp"
#include "World.hpp"

#include "triglav/Math.hpp"

namespace triglav::world {

using namespace name_literals;

#define TG_TYPE(NS) NS(triglav, NS(world, Mesh))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, triglav::MeshName)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(triglav, NS(world, Armature))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, triglav::ArmatureName)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(triglav, NS(world, Tag))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(tag, triglav::Name)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) NS(triglav, NS(world, EntityLabel))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(label, std::string)
TG_META_CLASS_END
#undef TG_TYPE

ComponentRegisterer transform_registerer{{
   .component_class = "triglav::Transform3D"_name,
   .data_size = sizeof(Transform3D),
   .data_alignment = alignof(Transform3D),
   .name = "Transform",
}};

ComponentRegisterer mesh_registerer{{
   .component_class = "triglav::world::Mesh"_name,
   .data_size = sizeof(Mesh),
   .data_alignment = alignof(Mesh),
   .name = "Mesh",
}};

ComponentRegisterer armature_registerer{{
   .component_class = "triglav::world::Armature"_name,
   .data_size = sizeof(Armature),
   .data_alignment = alignof(Armature),
   .name = "Armature",
}};

ComponentRegisterer tag_registerer{{
   .component_class = "triglav::world::Tag"_name,
   .data_size = sizeof(Tag),
   .data_alignment = alignof(Tag),
   .name = "Tag",
}};

ComponentRegisterer entity_label_registerer{{
   .component_class = "triglav::world::EntityLabel"_name,
   .data_size = sizeof(EntityLabel),
   .data_alignment = alignof(EntityLabel),
   .name = "Entity Label",
}};

void ensure_component_registration()
{
   (void)transform_registerer;
   (void)mesh_registerer;
   (void)armature_registerer;
   (void)tag_registerer;
   (void)entity_label_registerer;
}

}// namespace triglav::world
