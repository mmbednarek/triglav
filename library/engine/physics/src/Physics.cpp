#include "Physics.hpp"

#include "triglav/world/ComponentManager.hpp"

namespace triglav::physics {

using namespace name_literals;

#define TG_TYPE(NS) NS(triglav, NS(physics, Movement))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(motion, triglav::Vector3)
TG_META_PROPERTY(is_on_ground, bool)
TG_META_CLASS_END
#undef TG_TYPE

world::ComponentRegisterer movement_registerer{{
   .component_class = "triglav::physics::Movement"_name,
   .data_size = sizeof(Movement),
   .data_alignment = alignof(Movement),
   .name = "Movement",
}};

#define TG_TYPE(NS) NS(triglav, NS(physics, CapsuleComponent))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(capsule, triglav::geometry::Capsule)
TG_META_CLASS_END
#undef TG_TYPE

world::ComponentRegisterer capsule_component_registerer{{
   .component_class = "triglav::physics::CapsuleComponent"_name,
   .data_size = sizeof(CapsuleComponent),
   .data_alignment = alignof(CapsuleComponent),
   .name = "CapsuleComponent",
}};

}// namespace triglav::physics
