#include <cstdio>
#include <cstdlib>
#include <iostream>


#include <cxxopts.hpp>

#include "ast.h"
#include "gen.h"

using namespace std;

extern FILE *yyin = NULL, *yyout = NULL;

extern int yyparse();

extern NBlock *programBlock;

int main(int argc, char **argv) {
  cxxopts::Options options("The xy Language Compiler",
                           "A toy complier based on LLVM JIT.");
  options.add_options()
      ("d,debug", "Enable debugging")
      ("f,file", "File name", cxxopts::value<std::string>())
      ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"));

  auto result = options.parse(argc, argv);
  if (result.count("f,file") > 0)
  {
     auto inputFilePath = result["f,file"].as<std::string>();
    
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
