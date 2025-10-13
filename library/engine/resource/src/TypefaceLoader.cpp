#include "TypefaceLoader.hpp"

namespace triglav::resource {

font::Typeface Loader<ResourceType::Typeface>::load_font(const font::FontManger& manager, const io::Path& path)
{
   return manager.create_typeface(path, 0);
}

}// namespace triglav::resource
