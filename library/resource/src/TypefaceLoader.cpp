#include "TypefaceLoader.h"

namespace triglav::resource {

font::Typeface Loader<ResourceType::Typeface>::load_font(const font::FontManger &manager,
                                                         const std::string_view path)
{
   return manager.create_typeface(path, 0);
}

}// namespace triglav::resource
