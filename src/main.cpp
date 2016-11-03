#include <iostream>
#include "gen.h"
#include "ast.h"

using namespace std;

extern int yyparse();

extern NBlock *programBlock;

int main(int argc, char **argv) {
    yyparse();
    cout << programBlock << endl;
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    CodeGenContext context;
    context.generateCode(*programBlock);
    context.runCode();

    std::cout << "Exiting...\n";

    return 0;
}
