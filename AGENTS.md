# xy-compiler

Toy compiler: Flex lexer (`src/lexical.l`), Bison parser (`src/syntactic.y`), AST
(`src/ast.h`), LLVM code generation and MCJIT (`src/gen.h`, `src/gen.cpp`), command line
entry (`src/main.cpp`) and the runtime functions programs can call (`src/runtime.cpp`).
Everything builds into one executable from the root `CMakeLists.txt`. README.md (Chinese) is
both the tutorial and the reference; keep its code snippets in sync with the sources.

## Commands

- Configure: `cmake -S . -B build -G Ninja [-DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm]`
- Build: `cmake --build build`
- Test: `ctest --test-dir build --output-on-failure`
- Format: `git ls-files '*.cpp' '*.h' | xargs clang-format -i` (CI checks with clang-format-18 `--dry-run --Werror`)

## Conventions

- The grammar must stay conflict-free: bison runs with `-Werror=conflicts-sr -Werror=conflicts-rr`.
- Tests are `tests/cases/*.xy` with `// expect-stdout:`, `// expect-stderr:`, `// expect-exit:` comments
  (see `tests/run_test.cmake`). Re-run CMake configure after adding a case. CLI tests are in `CMakeLists.txt`.
- Source errors are reported as `line N: error: ...` with exit code 1; semantic errors throw `CodeGenError`.
- A new runtime function goes in `src/runtime.cpp` (`extern "C"`, `long long` arguments and result)
  and must be registered with `DynamicLibrary::AddSymbol` in `src/main.cpp`.
- Supported LLVM: 14+ (verified on 17 and 21). Windows is not covered by CI.
