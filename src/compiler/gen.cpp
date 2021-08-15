#include "gen.h"
#include "ast.h"
#include "syntactic.hpp"

#include <exception>

#include <llvm/ADT/Twine.h>
#include <llvm/IR/LegacyPassManager.h>

using namespace std;

LLVMContext TheContext;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock &root) {
  std::cout << "Generating code...\n";

  /* Create the top level interpreter function to call as entry */
  vector<Type *> argTypes;
  FunctionType *ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()),
                                          makeArrayRef(argTypes), false);
  mainFunction =
      Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
  BasicBlock *bblock =
      BasicBlock::Create(getGlobalContext(), "entry", mainFunction, 0);

  /* Push a new variable/block context */
  pushBlock(bblock);
  root.codeGen(*this); /* emit bytecode for the toplevel block */
  ReturnInst::Create(getGlobalContext(), bblock);
  popBlock();

  /* Print the bytecode in a human-readable format
     to see if our program compiled properly
   */
  std::cout << "Code is generated.\n";
  // PassManager<Module> pm;
  // AnalysisManager<Module> am;
  // pm.addPass(PrintModulePass(outs()));
  // pm.run(*module, am);
  llvm::legacy::PassManager pm;
  pm.add(createPrintModulePass(outs()));
  pm.run(*module);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() {
  try {
    std::cout << "Running code:\n";
    ExecutionEngine *ee = EngineBuilder(unique_ptr<Module>(module)).create();
    ee->finalizeObject();
    vector<GenericValue> noargs;
    GenericValue v = ee->runFunction(mainFunction, noargs);
    delete ee;
    return v;
  } catch (std::exception &ex) {
    std::cout << ex.what() << std::endl;
  }
}

/* -- Code Generation -- */

Value *NInteger::codeGen(CodeGenContext &context) {
  std::cout << "Creating integer: " << value << endl;
  return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value *NIdentifier::codeGen(CodeGenContext &context) {
  std::cout << "Creating identifier reference: " << name << endl;
  if (context.locals().find(name) == context.locals().end()) {
    std::cout << "undeclared variable " << name << endl;
    std::cout << "Creating variable declaration " << name << endl;
    AllocaInst *alloc = new AllocaInst(Type::getInt64Ty(getGlobalContext()), 0u,
                                       name.c_str(), context.currentBlock());
    context.locals()[name] = alloc;
  }

#if LLVM_VERSION_MAJOR == 12
  const auto &local = context.locals()[name];
  return new LoadInst(cast<PointerType>(local->getType())->getElementType(),
                      local, llvm::Twine(""), false, context.currentBlock());
#else
  return new LoadInst(context.locals()[name], "", false,
                      context.currentBlock());
#endif
}

Value *NMethodCall::codeGen(CodeGenContext &context) {
  Function *function = context.module->getFunction(id.name.c_str());
  if (function == NULL) {
    std::cerr << "no such function " << id.name << endl;
  }
  std::vector<Value *> args;
  ExpressionList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    args.push_back((**it).codeGen(context));
  }
  CallInst *call = CallInst::Create(function, makeArrayRef(args), "",
                                    context.currentBlock());
  std::cout << "Creating method call: " << id.name << endl;
  return call;
}

Value *NBinaryOperator::codeGen(CodeGenContext &context) {
  std::cout << "Creating binary operation " << op << endl;
  Instruction::BinaryOps instr;
  switch (op) {
  case TPLUS:
    instr = Instruction::Add;
    goto math;
  case TMINUS:
    instr = Instruction::Sub;
    goto math;
  case TMUL:
    instr = Instruction::Mul;
    goto math;
  case TDIV:
    instr = Instruction::SDiv;
    goto math;

    /* TODO comparison */
  }

  return NULL;
math:
  return BinaryOperator::Create(instr, lhs.codeGen(context),
                                rhs.codeGen(context), "",
                                context.currentBlock());
}

Value *NAssignment::codeGen(CodeGenContext &context) {
  std::cout << "Creating assignment for " << lhs.name << endl;
  if (context.locals().find(lhs.name) == context.locals().end()) {
    std::cout << "undeclared variable " << lhs.name << endl;
    std::cout << "Creating variable declaration " << lhs.name << endl;
    AllocaInst *alloc =
        new AllocaInst(Type::getInt64Ty(getGlobalContext()), 0u,
                       lhs.name.c_str(), context.currentBlock());
    context.locals()[lhs.name] = alloc;
  }
  return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false,
                       context.currentBlock());
}

Value *NBlock::codeGen(CodeGenContext &context) {
  StatementList::const_iterator it;
  Value *last = NULL;
  for (it = statements.begin(); it != statements.end(); it++) {
    std::cout << "Generating code for " << typeid(**it).name() << endl;
    last = (**it).codeGen(context);
  }
  std::cout << "Creating block" << endl;
  return last;
}

Value *NExpressionStatement::codeGen(CodeGenContext &context) {
  std::cout << "Generating code for " << typeid(expression).name() << endl;
  return expression.codeGen(context);
}

Value *NReturnStatement::codeGen(CodeGenContext &context) {
  std::cout << "Generating return code for " << typeid(expression).name()
            << endl;
  Value *returnValue = expression.codeGen(context);
  context.setCurrentReturnValue(returnValue);
  return returnValue;
}

Value *NExternDeclaration::codeGen(CodeGenContext &context) {
  vector<Type *> argTypes;
  VariableList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    argTypes.push_back(Type::getInt64Ty(getGlobalContext()));
  }
  FunctionType *ftype = FunctionType::get(Type::getInt64Ty(getGlobalContext()),
                                          makeArrayRef(argTypes), false);
  Function *function = Function::Create(ftype, GlobalValue::ExternalLinkage,
                                        id.name.c_str(), context.module);
  return function;
}

Value *NFunctionDeclaration::codeGen(CodeGenContext &context) {
  vector<Type *> argTypes;
  VariableList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    argTypes.push_back(Type::getInt64Ty(getGlobalContext()));
  }
  FunctionType *ftype = FunctionType::get(Type::getInt64Ty(getGlobalContext()),
                                          makeArrayRef(argTypes), false);
  Function *function = Function::Create(ftype, GlobalValue::InternalLinkage,
                                        id.name.c_str(), context.module);
  BasicBlock *bblock =
      BasicBlock::Create(getGlobalContext(), "entry", function, 0);

  context.pushBlock(bblock);

  Function::arg_iterator argsValues = function->arg_begin();
  Value *argumentValue;

  for (it = arguments.begin(); it != arguments.end(); it++) {
    (**it).codeGen(context);

    argumentValue = &*argsValues++;
    argumentValue->setName((*it)->name.c_str());
    StoreInst *inst = new StoreInst(
        argumentValue, context.locals()[(*it)->name], false, bblock);
  }

  block.codeGen(context);
  ReturnInst::Create(getGlobalContext(), context.getCurrentReturnValue(),
                     bblock);

  context.popBlock();
  std::cout << "Creating function: " << id.name << endl;
  return function;
}
