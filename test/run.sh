#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
cc -I../main -std=c11 -Wall -Wextra -Werror -O1 -o "$OUT/test_story" test_story.c ../main/story.c
mkdir -p "$OUT/card/1002.bin"
for name in 8670.bin 4826.BIN 1203.bin 0089.bin 1003.tmp 1004.bin.bak abc9.bin; do
    printf '\001\000\007' > "$OUT/card/$name"
done
touch "$OUT/card/1000.bin"
dd if=/dev/zero of="$OUT/card/1001.bin" bs=1048576 count=3 2>/dev/null
"$OUT/test_story" "$OUT/card"
cc -I../main -std=c11 -Wall -Wextra -Werror -O1 -o "$OUT/test_sleep_timer" ../prototype/test_sleep_timer.c ../prototype/sleep_timer.c
"$OUT/test_sleep_timer"
printf 'Story and sleep timer tests passed\n'
