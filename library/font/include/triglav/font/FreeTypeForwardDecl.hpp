#pragma once

#define DECL_FREE_TYPE(Name) \
   struct FT_##Name##Rec_;   \
   typedef struct FT_##Name##Rec_* FT_##Name;

DECL_FREE_TYPE(Library)
DECL_FREE_TYPE(Face)

#undef DECL_FREE_TYPE