#pragma once

#include <vector>

namespace triglav {

template<typename TElement>
class RangedArray
{
 public:
   using IndexType = u32;

   struct Pair
   {
      IndexType count;
      TElement element;
   };

   TElement at(IndexType offset) const
   {
      auto at = m_pairs.begin();
      while (at != m_pairs.end() && offset > at->count) {
         offset -= at->count;
         ++at;
      }
      assert(at != m_pairs.end());
      return at->element;
   }

 private:
   std::vector<Pair> m_pairs;
};


}// namespace triglav