#!/usr/bin/env bash

touch tmp_names

for line in $(grep -rohE '"[A-Za-z_/.]*"_name' $(find game library -regextype posix-extended -regex '^.*\.(cpp|h|hpp)$'));
do
  if [[ $line =~ \"([A-Za-z_/.]*)\"_name ]]; then
    echo "${BASH_REMATCH[1]}" >> tmp_names
  fi
done

for line in $(grep -rohE '"[A-Za-z_/]*\.[a-z]*"_rc' $(find game library -regextype posix-extended -regex '^.*\.(cpp|h|hpp)$'));
do
  if [[ $line =~ \"([A-Za-z_/]*)\.([a-z]*)\"_rc ]]; then
    echo "${BASH_REMATCH[1]}" >> tmp_names
  fi
done


echo '#include "NameResolution.hpp"'
echo ''
echo '#include <map>'
echo ''
echo 'namespace triglav {'
echo ''

echo 'namespace {'
echo ''
echo 'using namespace std::string_view_literals;'
echo 'using namespace name_literals;'
echo ''

echo "std::map<Name, std::string_view> KnownNames {"

for line in $(cat tmp_names | sort | uniq); do
  echo "   {\"$line\"_name, \"$line\"sv},"
done

echo '};'

echo ''
echo '} // namespace'
echo ''
echo 'std::string_view resolve_name(const Name name)'
echo '{'
echo '    const auto it = KnownNames.find(name);'
echo '    return it == KnownNames.end() ? std::string_view{} : it->second;'
echo '}'
echo ''
echo '} // namespace triglav'
echo ''

rm tmp_names