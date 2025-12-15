#!/usr/bin/env bash

touch tmp_names

for line in $(grep -rohE '"[A-Za-z0-9_/]*\.[a-z]*"_rc' $(find game library tool -regextype posix-extended -regex '^.*\.(cpp|h|hpp)$' | grep -v '/test/'));
do
  if [[ $line =~ \"([A-Za-z0-9_/]*\.[a-z]*)\"_rc ]]; then
    echo "${BASH_REMATCH[1]}" >> tmp_names
  fi
done

cat tmp_names | sort | uniq >collected_res
yq -r '.resources[]' < game/demo/content/index_base.yaml | sort | uniq >base_res

echo "resources:"
for line in $(comm -23 collected_res base_res); do
  echo "  - \"$line\""
done

rm tmp_names
rm collected_res
rm base_res