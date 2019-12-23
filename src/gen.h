// #include <llvm/Bitcode/ReaderWriter.h>
// #include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <stack>
#include <typeinfo>

using namespace llvm;

extern LLVMContext TheContext;

inline LLVMContext &getGlobalContext() { return TheContext; }

class NBlock;

class CodeGenBlock {
public:
  BasicBlock *block;

  Value *returnValue;
  std::map<std::string, Value *> locals;
};

class CodeGenContext {
private:
  std::stack<CodeGenBlock *> blocks;
  Function *mainFunction;

public:
  Module *module;

public:
  CodeGenContext() { module = new Module("main", getGlobalContext()); }

  void generateCode(NBlock &root);

  GenericValue runCode();

  inline std::map<std::string, Value *> &locals() {
    return blocks.top()->locals;
  }

  inline BasicBlock *currentBlock() { return blocks.top()->block; }

  inline void pushBlock(BasicBlock *block) {
    blocks.push(new CodeGenBlock());
    blocks.top()->returnValue = NULL;
    blocks.top()->block = block;
  }

  inline void popBlock() {
    CodeGenBlock *top = blocks.top();
    blocks.pop();
    delete top;
  }

  inline void setCurrentReturnValue(Value *value) {
    blocks.top()->returnValue = value;
  }

  inline Value *getCurrentReturnValue() { return blocks.top()->returnValue; }
};
