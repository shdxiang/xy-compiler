#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <system_error>

#include <cxxopts.hpp>

#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <libxy/version.h>

#include "ast.h"
#include "gen.h"

#include "version.hpp"

extern FILE *yyin;
extern int yydebug;

extern int yyparse();

extern NBlock *programBlock;

static bool writeIR(const CodeGenContext &context, const std::string &path) {
  if (path == "-") {
    context.printIR(llvm::outs());
    return true;
  }
  std::error_code error;
  llvm::raw_fd_ostream out(path, error, llvm::sys::fs::OF_None);
  if (error) {
    std::cerr << "error: cannot open output file '" << path
              << "': " << error.message() << std::endl;
    return false;
  }
  context.printIR(out);
  return true;
}

/* Parse the program from yyin, compile it, then run it with the JIT */
static int compileAndRun(const cxxopts::ParseResult &args) {
  yydebug = args.count("debug") ? 1 : 0;
  int parseResult = yyparse();
  if (yyin != stdin) {
    std::fclose(yyin);
  }
  if (parseResult != 0 || !programBlock) {
    return 1;
  }
  std::unique_ptr<NBlock> program(programBlock);
  programBlock = nullptr;

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  CodeGenContext context(args.count("verbose") > 0);
  try {
    context.generateCode(*program);
  } catch (const CodeGenError &ex) {
    std::cerr << "line " << ex.line << ": error: " << ex.what() << std::endl;
    return 1;
  } catch (const std::exception &ex) {
    std::cerr << "internal error: " << ex.what() << std::endl;
    return 1;
  }

  if (args.count("output") &&
      !writeIR(context, args["output"].as<std::string>())) {
    return 1;
  }
  if (args.count("no-run")) {
    return 0;
  }

  /* Make the symbols of this process (including libxy) visible to the JIT */
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
  try {
    return static_cast<int>(context.runCode());
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << std::endl;
    return 1;
  }
}

int main(int argc, char **argv) {
  cxxopts::Options options("xy-compiler", "A toy compiler based on LLVM JIT.");
  options.positional_help("[file]").show_positional_help();
  // clang-format off
  options.add_options()
      ("d,debug", "Print the parser trace")
      ("f,file", "Input file name (default: standard input)",
       cxxopts::value<std::string>())
      ("o,output", "Write the generated LLVM IR to a file ('-' for stdout)",
       cxxopts::value<std::string>())
      ("n,no-run", "Only compile, do not run the program")
      ("v,verbose", "Print code generation steps")
      ("h,help", "Print usage")
      ("version", "Print version");
  // clang-format on
  options.parse_positional({"file"});

  std::unique_ptr<cxxopts::ParseResult> result;
  try {
    result = std::make_unique<cxxopts::ParseResult>(options.parse(argc, argv));
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << std::endl;
    return 2;
  }
  const auto &args = *result;

  if (args.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  if (args.count("version")) {
    std::cout << "Version: " << VER << std::endl;
    char buffer[256];
    if (get_libxy_version(buffer, sizeof(buffer)) != 0) {
      std::cerr << "failed to get libxy version." << std::endl;
      return 1;
    }
    std::cout << "libxy version: " << buffer << std::endl;
    return 0;
  }

  yyin = stdin;
  if (args.count("file")) {
    const auto &inputFilePath = args["file"].as<std::string>();
    yyin = std::fopen(inputFilePath.c_str(), "r");
    if (!yyin) {
      std::cerr << "error: cannot open input file '" << inputFilePath
                << "': " << std::strerror(errno) << std::endl;
      return 1;
    }
  }

  return compileAndRun(args);
}
