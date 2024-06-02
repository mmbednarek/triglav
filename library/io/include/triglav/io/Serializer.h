#pragma once

#include "Stream.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace triglav::io {

class Serializer {
 public:
   explicit Serializer(IWriter& writer);

   Result<void> write_float32(float value);
   Result<void> write_vec3(glm::vec3 value);
   Result<void> write_vec4(glm::vec4 value);

 private:
   Result<void> add_padding(u32 alignment);

   IWriter& m_writer;
   u32 m_bytesWritten{};
};

}