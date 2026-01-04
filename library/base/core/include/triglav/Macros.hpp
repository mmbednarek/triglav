#pragma once

#define TG_CONCAT_INTERNAL(x, y) x##y
#define TG_CONCAT(x, y) TG_CONCAT_INTERNAL(x, y)

#define TG_STRING_INTERNAL(x) #x
#define TG_STRING(x) TG_STRING_INTERNAL(x)

#define TG_DELETE_COPY(class_name)         \
   class_name(const class_name&) = delete; \
   class_name& operator=(const class_name&) = delete;

#define TG_DELETE_MOVE(class_name)             \
   class_name(class_name&&) noexcept = delete; \
   class_name& operator=(class_name&&) noexcept = delete;

#define TG_DELETE_ALL(class_name) \
   TG_DELETE_COPY(class_name)     \
   TG_DELETE_MOVE(class_name)
