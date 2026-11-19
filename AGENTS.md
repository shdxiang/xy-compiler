# xy-compiler

The Flex lexer and Bison parser build an AST. The compiler lowers it to LLVM IR
and executes it with MCJIT. The [Chinese tutorial](README.md) includes a
[source map](README.md#项目结构); keep its examples in sync with the implementation.

## Commands

Run from the repository root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To select a specific LLVM installation, add `-DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm`
to the configure command, adjusting the path as needed. Use LLVM 14 or later.
On Windows, run the Linux commands inside WSL; native builds are unsupported.

## Conventions

- **Grammar:** Keep the Bison grammar free of shift/reduce and reduce/reduce conflicts. The build treats both as errors.
- **Tests:** Add language cases under `tests/cases/` with the [expected-output directives](README.md#测试). Define CLI tests in the root `CMakeLists.txt`.
- **Diagnostics:** Report source errors on stderr with a line number and exit code 1. Use `CodeGenError` for semantic errors.
- **Runtime:** Define new functions in `src/runtime.cpp` with C linkage and `long long` parameters and result. Register each function with `DynamicLibrary::AddSymbol` in `src/main.cpp`.
- **Formatting:** Run `git ls-files '*.cpp' '*.h' | xargs clang-format -i`. CI checks formatting with `clang-format-18`.
