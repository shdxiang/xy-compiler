#include "gen.h"
#include "ast.h"
#include "syntactic.hpp"

#include <set>
#include <vector>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Verifier.h>

using namespace llvm;

CodeGenContext::CodeGenContext(bool verbose)
    : module(std::make_unique<Module>("main", llvmContext)),
      builder(llvmContext), verbose(verbose) {}

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock &root) {
  trace("Generating code...");

  /* Create the top level function to call as entry; its return value is the
     value of a top level return statement, or 0 */
  FunctionType *ftype = FunctionType::get(intType(), false);
  mainFunction =
      Function::Create(ftype, GlobalValue::ExternalLinkage, "main", *module);
  builder.SetInsertPoint(
      BasicBlock::Create(llvmContext, "entry", mainFunction));

  root.codeGen(*this); /* emit bytecode for the toplevel block */
  finishFunction(mainFunction);

  std::string errors;
  raw_string_ostream errorStream(errors);
  if (verifyModule(*module, &errorStream)) {
    throw std::logic_error("generated invalid module:\n" + errorStream.str());
  }
  trace("Code is generated.");
}

void CodeGenContext::printIR(raw_ostream &out) const {
  module->print(out, nullptr);
}

/* Executes the AST by running the main function */
int64_t CodeGenContext::runCode() {
  trace("Running code:");
  std::string error;
  std::unique_ptr<ExecutionEngine> ee(EngineBuilder(std::move(module))
                                          .setEngineKind(EngineKind::JIT)
                                          .setErrorStr(&error)
                                          .create());
  if (!ee) {
    throw std::runtime_error("failed to create the JIT: " + error);
  }
  ee->finalizeObject();
  GenericValue result = ee->runFunction(mainFunction, {});
  return result.IntVal.getSExtValue();
}

AllocaInst *CodeGenContext::createLocal(const std::string &name,
                                        const std::string &irName) {
  BasicBlock &entry = builder.GetInsertBlock()->getParent()->getEntryBlock();
  /* Keep the allocas together, in declaration order */
  auto position = entry.begin();
  while (position != entry.end() && isa<AllocaInst>(*position)) {
    ++position;
  }
  IRBuilder<> entryBuilder(&entry, position);
  AllocaInst *alloca = entryBuilder.CreateAlloca(
      intType(), nullptr, irName.empty() ? name : irName);
  locals[name] = alloca;
  return alloca;
}

void CodeGenContext::finishFunction(Function *function) {
  /* Functions without a return statement return 0 */
  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateRet(builder.getInt64(0));
  }

  /* Drop the blocks holding dead code after return statements */
  std::vector<BasicBlock *> dead;
  for (BasicBlock &block : *function) {
    if (&block != &function->getEntryBlock() && pred_empty(&block)) {
      dead.push_back(&block);
    }
  }
  for (BasicBlock *block : dead) {
    block->dropAllReferences();
  }
  for (BasicBlock *block : dead) {
    block->eraseFromParent();
  }

  std::string errors;
  raw_string_ostream errorStream(errors);
  if (verifyFunction(*function, &errorStream)) {
    throw std::logic_error("generated invalid function '" +
                           function->getName().str() + "':\n" +
                           errorStream.str());
  }
}

/* -- Code Generation -- */

Value *NInteger::codeGen(CodeGenContext &context) {
  context.trace("Creating integer: " + std::to_string(value));
  return context.builder.getInt64(value);
}

Value *NIdentifier::codeGen(CodeGenContext &context) {
  context.trace("Creating identifier reference: " + name);
  auto it = context.locals.find(name);
  if (it == context.locals.end()) {
    throw CodeGenError(line, "undeclared variable '" + name + "'");
  }
  return context.builder.CreateLoad(context.intType(), it->second, name);
}

Value *NMethodCall::codeGen(CodeGenContext &context) {
  context.trace("Creating method call: " + id->name);
  Function *function = context.module->getFunction(id->name);
  if (!function || context.isEntryFunction(function)) {
    throw CodeGenError(line, "call to undefined function '" + id->name + "'");
  }
  if (function->arg_size() != arguments.size()) {
    throw CodeGenError(line, "function '" + id->name + "' expects " +
                                 std::to_string(function->arg_size()) +
                                 " argument(s), got " +
                                 std::to_string(arguments.size()));
  }
  std::vector<Value *> args;
  for (auto &argument : arguments) {
    args.push_back(argument->codeGen(context));
  }
  return context.builder.CreateCall(function, args);
}

Value *NBinaryOperator::codeGen(CodeGenContext &context) {
  context.trace("Creating binary operation " + std::to_string(op));
  /* Evaluate the operands left to right */
  Value *left = lhs->codeGen(context);
  Value *right = rhs->codeGen(context);
  IRBuilder<> &builder = context.builder;
  switch (op) {
  case TPLUS:
    return builder.CreateAdd(left, right);
  case TMINUS:
    return builder.CreateSub(left, right);
  case TMUL:
    return builder.CreateMul(left, right);
  case TDIV:
    return builder.CreateSDiv(left, right);
  case TCEQ:
    return builder.CreateZExt(builder.CreateICmpEQ(left, right),
                              context.intType());
  case TCNE:
    return builder.CreateZExt(builder.CreateICmpNE(left, right),
                              context.intType());
  }
  throw std::logic_error("unknown binary operator " + std::to_string(op));
}

Value *NAssignment::codeGen(CodeGenContext &context) {
  context.trace("Creating assignment for " + lhs->name);
  /* Evaluate the value first, so `x = x` does not declare x */
  Value *value = rhs->codeGen(context);
  auto it = context.locals.find(lhs->name);
  AllocaInst *variable =
      it != context.locals.end() ? it->second : context.createLocal(lhs->name);
  context.builder.CreateStore(value, variable);
  return value;
}

Value *NBlock::codeGen(CodeGenContext &context) {
  Value *last = nullptr;
  for (auto &statement : statements) {
    last = statement->codeGen(context);
  }
  context.trace("Creating block");
  return last;
}

Value *NExpressionStatement::codeGen(CodeGenContext &context) {
  return expression->codeGen(context);
}

Value *NReturnStatement::codeGen(CodeGenContext &context) {
  context.trace("Generating return code");
  Value *returnValue = expression->codeGen(context);
  IRBuilder<> &builder = context.builder;
  builder.CreateRet(returnValue);
  /* Code after a return is unreachable; give it a block of its own so it
     still produces valid IR. finishFunction() removes it. */
  builder.SetInsertPoint(
      BasicBlock::Create(context.llvmContext, "after_return",
                         builder.GetInsertBlock()->getParent()));
  return returnValue;
}

Value *NExternDeclaration::codeGen(CodeGenContext &context) {
  context.trace("Creating extern declaration: " + id->name);
  Function *existing = context.module->getFunction(id->name);
  if (existing) {
    if (existing->isDeclaration() && existing->arg_size() == arguments.size()) {
      return existing;
    }
    throw CodeGenError(line, "conflicting declaration of '" + id->name + "'");
  }
  std::vector<Type *> argTypes(arguments.size(), context.intType());
  FunctionType *ftype = FunctionType::get(context.intType(), argTypes, false);
  return Function::Create(ftype, GlobalValue::ExternalLinkage, id->name,
                          *context.module);
}

Value *NFunctionDeclaration::codeGen(CodeGenContext &context) {
  context.trace("Creating function: " + id->name);
  if (context.module->getFunction(id->name)) {
    throw CodeGenError(line, "redefinition of '" + id->name + "'");
  }
  std::set<std::string> names;
  for (auto &argument : arguments) {
    if (!names.insert(argument->name).second) {
      throw CodeGenError(argument->line, "duplicate parameter '" +
                                             argument->name + "' in '" +
                                             id->name + "'");
    }
  }

  std::vector<Type *> argTypes(arguments.size(), context.intType());
  FunctionType *ftype = FunctionType::get(context.intType(), argTypes, false);
  Function *function = Function::Create(ftype, GlobalValue::InternalLinkage,
                                        id->name, *context.module);

  /* Functions may be declared inside other code; save the enclosing state */
  IRBuilder<> &builder = context.builder;
  BasicBlock *enclosingBlock = builder.GetInsertBlock();
  auto enclosingLocals = std::move(context.locals);
  context.locals.clear();

  builder.SetInsertPoint(
      BasicBlock::Create(context.llvmContext, "entry", function));
  auto argumentValue = function->arg_begin();
  for (auto &argument : arguments) {
    argumentValue->setName(argument->name);
    builder.CreateStore(
        &*argumentValue++,
        context.createLocal(argument->name, argument->name + ".addr"));
  }

  block->codeGen(context);
  context.finishFunction(function);

  context.locals = std::move(enclosingLocals);
  builder.SetInsertPoint(enclosingBlock);
  return function;
}
