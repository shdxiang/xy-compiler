#ifndef C7B2E5A0_3F4D_4E8B_9A61_2D5C8F0B1E37
#define C7B2E5A0_3F4D_4E8B_9A61_2D5C8F0B1E37

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

class NBlock;

/* A semantic error in the source program, reported with its line number */
class CodeGenError : public std::runtime_error {
public:
  int line;
  CodeGenError(int line, const std::string &message)
      : std::runtime_error(message), line(line) {}
};

class CodeGenContext {
public:
  llvm::LLVMContext llvmContext;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;
  /* Variables of the function currently being generated */
  std::map<std::string, llvm::AllocaInst *> locals;
  bool verbose;

  explicit CodeGenContext(bool verbose = false);

  /* Compile the AST into the module; throws CodeGenError */
  void generateCode(NBlock &root);

  void printIR(llvm::raw_ostream &out) const;

  /* JIT-compile the module and run it, returning the program's return value.
     The module is handed over to the JIT, so this can only be called once. */
  int64_t runCode();

  llvm::Type *intType() { return builder.getInt64Ty(); }

  /* Create a stack slot for a variable in the entry block of the current
     function */
  llvm::AllocaInst *createLocal(const std::string &name,
                                const std::string &irName = "");

  /* Terminate the current block if needed and verify the function */
  void finishFunction(llvm::Function *function);

  bool isEntryFunction(const llvm::Function *function) const {
    return function == mainFunction;
  }

  void trace(const std::string &message) const {
    if (verbose) {
      llvm::errs() << message << "\n";
    }
  }

private:
  llvm::Function *mainFunction = nullptr;
};

#endif /* C7B2E5A0_3F4D_4E8B_9A61_2D5C8F0B1E37 */
