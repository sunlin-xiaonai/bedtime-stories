#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
cc -I../main -std=c11 -Wall -Wextra -Werror -O1 -o "$OUT/test_story" test_story.c ../main/story.c
"$OUT/test_story"
cc -I../main -std=c11 -Wall -Wextra -Werror -O1 -o "$OUT/test_sleep_timer" ../prototype/test_sleep_timer.c ../prototype/sleep_timer.c
"$OUT/test_sleep_timer"
printf 'Story and sleep timer tests passed\n'
