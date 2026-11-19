#!/bin/bash
# Formats the C++ sources with clang-format (style in .clang-format).
# Usage: scripts/format.sh [--check]
# Set CLANG_FORMAT to use a specific clang-format binary.
set -euo pipefail

cd "$(dirname "$0")/.."
CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

mode=(-i)
if [[ "${1:-}" == "--check" ]]; then
    mode=(--dry-run --Werror)
fi

git ls-files -co --exclude-standard '*.cpp' '*.h' '*.hpp' '*.h.in' '*.hpp.in' |
    xargs "$CLANG_FORMAT" --assume-filename=source.h "${mode[@]}"
