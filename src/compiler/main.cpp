#if defined(__STDC_LIB_EXT1__)
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <cxxopts.hpp>

#include "ast.h"
#include "gen.h"

#include "version.hpp"

using namespace std;

extern FILE *yyin, *yyout;

extern int yyparse();

extern NBlock *programBlock;

int main(int argc, char **argv) {
  cxxopts::Options options("The xy Language Compiler",
                           "A toy complier based on LLVM JIT.");
  options.add_options()("d,debug", "Enable debugging")(
      "f,file", "Input file name", cxxopts::value<std::string>())(
      "o,output", "Output file name", cxxopts::value<std::string>())(
      "v,verbose", "Verbose output",
      cxxopts::value<bool>()->default_value("false"))("h,help", "Print usage")(
      "version", "Print version");

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  if (result.count("version")) {
    std::cout << "Version: " << VER << std::endl;
    return 0;
  }
  if (result.count("file")) {
    auto inputFilePath = result["file"].as<std::string>();
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
    fopen_s(&yyin, inputFilePath.c_str(), "r");
#else
    yyin = fopen(inputFilePath.c_str(), "r");
#endif
  }
  if (result.count("output")) {
    auto outputFilePath = result["output"].as<std::string>();
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
    fopen_s(&yyout, outputFilePath.c_str(), "w");
#else
    yyout = fopen(outputFilePath.c_str(), "w");
#endif
  }

  yyparse();
  std::cout << programBlock << endl;
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  CodeGenContext context;
  context.generateCode(*programBlock);
  context.runCode();

  std::cout << "Exiting...\n";

  return 0;
}
