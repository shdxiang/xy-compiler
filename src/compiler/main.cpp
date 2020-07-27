#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <cxxopts.hpp>

#include "ast.h"
#include "gen.h"

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
      cxxopts::value<bool>()->default_value("false"))("h,help", "Print usage");

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    exit(0);
  }
  if (result.count("f,file") > 0) {
    auto inputFilePath = result["f,file"].as<std::string>();
    fopen_s(&yyin, inputFilePath.c_str(), "r");
  }
  if (result.count("o,output") > 0) {
    auto outputFilePath = result["o,output"].as<std::string>();
    fopen_s(&yyout, outputFilePath.c_str(), "w");
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
