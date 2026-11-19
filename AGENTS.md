# xy-compiler

Toy compiler: Flex lexer (`src/parser/lexical.l`), Bison parser (`src/parser/syntactic.y`),
AST (`src/parser/ast.h`), LLVM code generation and MCJIT (`src/compiler/gen.cpp`), runtime
library libxy (`src/runtime`). README.md (Chinese) is both the tutorial and the reference; keep
its code snippets in sync with the sources.

## Commands

- Configure: `cmake -S . -B build -G Ninja [-DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm]`
- Build: `cmake --build build`
- Test: `ctest --test-dir build --output-on-failure`
- Format: `scripts/format.sh` (check only: `scripts/format.sh --check`; set `CLANG_FORMAT` to pick a binary, CI uses clang-format-18)

## Conventions

- The grammar must stay conflict-free: bison runs with `-Werror=conflicts-sr -Werror=conflicts-rr`.
- Tests are `tests/cases/*.xy` with `// expect-stdout:`, `// expect-stderr:`, `// expect-exit:` comments
  (see `tests/run_test.cmake`). Re-run CMake configure after adding a case.
- Source errors are reported as `line N: error: ...` with exit code 1; semantic errors throw `CodeGenError`.
- Supported LLVM: 14+ (verified on 17 and 21).
