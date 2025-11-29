#pragma once

#include "Stream.hpp"

namespace triglav::io {

template<typename TChar>
class WriterIterator
{
 public:
   using iterator_category = std::output_iterator_tag;
   using value_type = TChar;
   using difference_type = ptrdiff_t;
   using pointer = TChar*;
   using reference = TChar&;

   explicit WriterIterator(IWriter& writer);

   WriterIterator& operator++();
   WriterIterator operator++(int);
   [[nodiscard]] reference operator*();
   [[nodiscard]] bool operator!=(const WriterIterator& other) const;

 private:
   IWriter* m_writer;
   TChar m_current_value;
};

template<typename TChar>
WriterIterator<TChar>::WriterIterator(IWriter& writer) :
    m_writer(&writer)
{
}

template<typename TChar>
WriterIterator<TChar>& WriterIterator<TChar>::operator++()
{
   auto value = static_cast<u8>(m_current_value);
   [[maybe_unused]] const auto res = m_writer->write({&value, 1});
   return *this;
}

template<typename TChar>
WriterIterator<TChar> WriterIterator<TChar>::operator++(int)
{
   auto copy = *this;
   this->operator++();
   return copy;
}

template<typename TChar>
TChar& WriterIterator<TChar>::operator*()
{
   return m_current_value;
}

template<typename TChar>
bool WriterIterator<TChar>::operator!=(const WriterIterator& /*other*/) const
{
   return true;
}

static_assert(std::output_iterator<WriterIterator<char>, char>);

}// namespace triglav::io