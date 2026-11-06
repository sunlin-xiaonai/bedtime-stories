#!/bin/sh
set -eu
cd "$(dirname "$0")"
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
cc -std=c11 -Wall -Wextra -Werror sleep_timer.c test_sleep_timer.c -o "$out/test_sleep_timer"
"$out/test_sleep_timer"
printf 'Sleep timer tests passed\n'
