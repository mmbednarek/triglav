#pragma once

#include "triglav/Int.hpp"

#include <concepts>
#include <span>

namespace triglav::memory {

template<typename TObject, template<typename> typename TAlloc>
concept Allocator = requires(TAlloc<TObject> alloc, MemorySize size, std::span<TObject> objects) {
   { alloc.allocate(size) } -> std::same_as<std::span<TObject>>;
   { alloc.release(objects) };
};

struct Area
{
   MemorySize size;
   MemorySize offset;
};

}// namespace triglav::memory
