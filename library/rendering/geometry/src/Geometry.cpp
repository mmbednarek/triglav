#include "Geometry.hpp"

#define TG_TYPE(ns) ns(triglav, ns(geometry, Capsule))
TG_META_CLASS_BEGIN
TG_META_PROPERTY(point_a, triglav::Vector3)
TG_META_PROPERTY(point_b, triglav::Vector3)
TG_META_PROPERTY(radius, float)
TG_META_CLASS_END
#undef TG_TYPE