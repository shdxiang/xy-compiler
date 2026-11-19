#ifndef A491CF4A_61DB_4F49_B826_A74408D8BC9D
#define A491CF4A_61DB_4F49_B826_A74408D8BC9D

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace llvm {
class Value;
}

extern int yylineno;

class CodeGenContext;
class NStatement;
class NExpression;
class NIdentifier;

typedef std::vector<std::unique_ptr<NStatement>> StatementList;
typedef std::vector<std::unique_ptr<NExpression>> ExpressionList;
typedef std::vector<std::unique_ptr<NIdentifier>> VariableList;

/* Base class of all AST nodes. Each node owns its children, so deleting the
   root block frees the whole tree. `line` is the source line used in error
   messages. */
class Node {
public:
  int line = yylineno;
  virtual ~Node() = default;
  virtual llvm::Value *codeGen(CodeGenContext &context) = 0;
};

class NExpression : public Node {};

class NStatement : public Node {};

class NInteger : public NExpression {
public:
  long long value;
  NInteger(long long value) : value(value) {}
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NIdentifier : public NExpression {
public:
  std::string name;
  NIdentifier(const std::string &name) : name(name) {}
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NMethodCall : public NExpression {
public:
  std::unique_ptr<NIdentifier> id;
  ExpressionList arguments;
  NMethodCall(NIdentifier *id, ExpressionList &&arguments)
      : id(id), arguments(std::move(arguments)) {
    line = this->id->line;
  }
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NBinaryOperator : public NExpression {
public:
  std::unique_ptr<NExpression> lhs;
  int op;
  std::unique_ptr<NExpression> rhs;
  NBinaryOperator(NExpression *lhs, int op, NExpression *rhs)
      : lhs(lhs), op(op), rhs(rhs) {}
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NAssignment : public NExpression {
public:
  std::unique_ptr<NIdentifier> lhs;
  std::unique_ptr<NExpression> rhs;
  NAssignment(NIdentifier *lhs, NExpression *rhs) : lhs(lhs), rhs(rhs) {
    line = this->lhs->line;
  }
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NBlock : public NExpression {
public:
  StatementList statements;
  NBlock() {}
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NExpressionStatement : public NStatement {
public:
  std::unique_ptr<NExpression> expression;
  NExpressionStatement(NExpression *expression) : expression(expression) {
    line = this->expression->line;
  }
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NReturnStatement : public NStatement {
public:
  std::unique_ptr<NExpression> expression;
  NReturnStatement(NExpression *expression) : expression(expression) {
    line = this->expression->line;
  }
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NExternDeclaration : public NStatement {
public:
  std::unique_ptr<NIdentifier> id;
  VariableList arguments;
  NExternDeclaration(NIdentifier *id, VariableList &&arguments)
      : id(id), arguments(std::move(arguments)) {
    line = this->id->line;
  }
  llvm::Value *codeGen(CodeGenContext &context) override;
};

class NFunctionDeclaration : public NStatement {
public:
  std::unique_ptr<NIdentifier> id;
  VariableList arguments;
  std::unique_ptr<NBlock> block;
  NFunctionDeclaration(NIdentifier *id, VariableList &&arguments, NBlock *block)
      : id(id), arguments(std::move(arguments)), block(block) {
    line = this->id->line;
  }
  llvm::Value *codeGen(CodeGenContext &context) override;
};

#endif /* A491CF4A_61DB_4F49_B826_A74408D8BC9D */
