#pragma once

#include "Path.hpp"
#include "Stream.hpp"
#include "triglav/EnumFlags.hpp"

#include <memory>
#include <string_view>
#include <variant>
#include <vector>

namespace triglav::io {

class IFile : public ISeekableStream
{
 public:
   [[nodiscard]] virtual Result<MemorySize> file_size() = 0;
};

using IFileUPtr = std::unique_ptr<IFile>;

enum class FileMode
{
   Read = (1 << 0),
   Write = (1 << 1),
   Create = (1 << 2),
   Append = (1 << 3),
};

TRIGLAV_DECL_FLAGS(FileMode)

Result<IFileUPtr> open_file(const Path& path, FileModeFlags mode);
std::vector<char> read_whole_file(const Path& path);
Result<std::monostate> remove_file(const Path& path);

}// namespace triglav::io