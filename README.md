# xy-compiler：实现一个简单的编译器

`xy-compiler` 是一个用 C++ 编写的教学编译器。它读取 `.xy` 源码，用 Flex 和 Bison 构建抽象语法树，再生成 LLVM IR。你可以查看这份 IR，也可以让 JIT 直接运行程序。

## 快速开始

安装好[依赖项](#安装依赖项)后，在源码目录下执行：

```bash
cmake -S . -B build
cmake --build build
./build/bin/xy-compiler demo.xy    # 输出 11
ctest --test-dir build --output-on-failure  # 运行测试
```

## 目录

- **运行示例**：[自定义语法](#自定义语法) · [工具和依赖项](#工具和依赖项) · [构建](#构建) · [查看输出](#编译测试)
- **理解实现**：[编译流程](#编译原理简介) · [词法分析器](#词法分析器) · [语法分析器](#语法分析器) · [生成 LLVM IR](#目标码生成)
- **查阅细节**：[命令行用法](#命令行用法) · [语言参考](#语言参考) · [项目结构](#项目结构) · [开发](#开发)

## 自定义语法

XY 只有一种类型：64 位有符号整数。变量和函数参数不需要类型声明。除四则运算外，还支持比较、赋值和函数调用，具体规则见[语言参考](#语言参考)。

下面是一个示例程序 [demo.xy](demo.xy)，它会打印 `4 * 5 - (4 + 5)` 的结果 `11`：

```c
extern printi(val)

sum(a, b) {
  return a + b
}

mult(a, b) {
  return a * b
}

printi(mult(4, 5) - sum(4, 5))
```

## 编译原理简介

以示例中的 `return a + b` 为例：

1. **词法分析**：词法分析器（lexer）把字符识别为词法单元（token）：关键字 `return`、标识符 `a`、运算符 `+`、标识符 `b`。
2. **语法分析**：语法分析器（parser）把这些词法单元组合成抽象语法树（AST）。返回语句包含一个加法表达式，表达式的两个子节点分别表示 `a` 和 `b`。
3. **代码生成**：编译器遍历 AST，生成读取参数、相加和返回结果的 LLVM IR 指令。运行程序时，JIT 再把 IR 编译成机器码；使用 `--no-run` 则在执行前停止。

## 工具和依赖项

| 工具 | 最低版本 | 用途 |
| --- | --- | --- |
| [Flex](https://github.com/westes/flex) | 2.6 | 根据词法规则生成词法分析器 |
| [Bison](https://www.gnu.org/software/bison/) | 3.0 | 根据语法规则生成语法分析器 |
| [LLVM](https://llvm.org/) | 14 | 构建并验证 IR，通过 JIT 执行程序 |
| [CMake](https://cmake.org) | 3.21 | 配置构建与测试 |

### 安装依赖项

1. Linux

   以 Ubuntu 为例，可以通过以下命令安装这些工具：

   ```bash
   sudo apt-get update
   sudo apt-get install -y git build-essential cmake ninja-build flex bison llvm-dev \
       zlib1g-dev libzstd-dev libedit-dev libxml2-dev libffi-dev libncurses-dev
   ```

2. macOS

   使用 **Homebrew** 安装依赖项：

   ```bash
   brew install llvm cmake flex bison
   ```

   本项目需要 Bison 3.0 或更高版本。为避免使用 macOS 自带的旧版 Bison，配置时将 Homebrew 安装的 Bison、Flex 和 LLVM 加入 CMake 的搜索路径：

   ```bash
   cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix bison);$(brew --prefix flex);$(brew --prefix llvm)"
   ```

3. Windows：不支持原生构建。在管理员 PowerShell 中运行 `wsl --install -d Ubuntu` 安装 WSL，然后打开 Ubuntu 终端，按上面的 Linux 步骤安装依赖项并执行[快速开始](#快速开始)中的命令。

## 词法分析器

[lexical.l](src/lexical.l) 告诉 Flex 如何识别关键字、标识符、整数和标点。例如，`"return"` 匹配关键字，而 `[0-9]+` 匹配一个或多个十进制数字：

```lex
%{
#include <string>
#include "ast.h"
#include "syntactic.hpp"

#define SAVE_TOKEN  yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t)    (yylval.token = t)
%}

%option noyywrap nounput noinput yylineno

%%

[ \t\r\n]               ;
"//".*                  ;
"extern"                return TOKEN(TEXTERN);
"return"                return TOKEN(TRETURN);
[a-zA-Z_][a-zA-Z0-9_]*  SAVE_TOKEN; return TIDENTIFIER;
[0-9]+                  SAVE_TOKEN; return TINTEGER;

"="                     return TOKEN(TEQUAL);
"=="                    return TOKEN(TCEQ);
"!="                    return TOKEN(TCNE);

"("                     return TOKEN(TLPAREN);
")"                     return TOKEN(TRPAREN);
"{"                     return TOKEN(TLBRACE);
"}"                     return TOKEN(TRBRACE);

","                     return TOKEN(TCOMMA);

"+"                     return TOKEN(TPLUS);
"-"                     return TOKEN(TMINUS);
"*"                     return TOKEN(TMUL);
"/"                     return TOKEN(TDIV);

.                       return TOKEN(TINVALID);

%%

```

这个文件被 2 个 `%%` 分成 3 部分：

1. 第 1 部分中 `%{` 与 `%}` 之间的 C++ 代码会被原样复制到 Flex 生成的源文件中，这里还可以定义宏和指定选项，如 `%option noyywrap`（不需要 `yywrap()` 函数）和 `yylineno`（记录行号，用于报错）；
2. 每条词法规则由模式和动作组成。匹配成功后，动作可以返回词法单元，也可以跳过输入（如空白或注释）。无法识别的字符返回 `TINVALID`，交由语法分析器报错。
3. 最后一部分用于添加辅助函数，本项目留空。

在 `src/` 目录中，以下命令可单独生成词法分析器；正常构建由 CMake 完成：

```bash
flex -o lexical.cpp lexical.l
```

生成的 `lexical.cpp` 提供 `yylex()`，供语法分析器逐个读取词法单元。`yytext` 是 Flex 提供的匹配文本；`TEXTERN` 和 `yylval` 则来自 Bison 生成的头文件。

## 语法分析器

语法分析器为源码中的表达式和语句创建 AST 节点。下图展示的是 [ast.h](src/ast.h) 中节点类型的继承关系，而不是某个程序的语法树：

```mermaid
flowchart LR
    Node(["Node"]) --> NExpression(["NExpression"]) & NStatement(["NStatement"])

    subgraph expressions [表达式]
        direction TB
        NInteger["NInteger<br/>42"]
        NIdentifier["NIdentifier<br/>x"]
        NMethodCall["NMethodCall<br/>f(a, b)"]
        NBinaryOperator["NBinaryOperator<br/>a + b"]
        NAssignment["NAssignment<br/>x = 1"]
        NBlock["NBlock<br/>{ x = 1 }"]
    end

    subgraph statements [语句]
        direction TB
        NExpressionStatement["NExpressionStatement<br/>f(1)"]
        NReturnStatement["NReturnStatement<br/>return x"]
        NExternDeclaration["NExternDeclaration<br/>extern f(a)"]
        NFunctionDeclaration["NFunctionDeclaration<br/>f(a) { return a }"]
    end

    NExpression --> NInteger & NIdentifier & NMethodCall & NBinaryOperator & NAssignment & NBlock
    NStatement --> NExpressionStatement & NReturnStatement & NExternDeclaration & NFunctionDeclaration

    classDef base fill:#ede7f6,stroke:#5e35b1,stroke-width:2px,color:#311b92
    classDef expr fill:#e3f2fd,stroke:#1e88e5,color:#0d47a1
    classDef stmt fill:#e8f5e9,stroke:#43a047,color:#1b5e20
    class Node,NExpression,NStatement base
    class NInteger,NIdentifier,NMethodCall,NBinaryOperator,NAssignment,NBlock expr
    class NExpressionStatement,NReturnStatement,NExternDeclaration,NFunctionDeclaration stmt
    style expressions fill:none,stroke:#1e88e5,stroke-dasharray:5 5
    style statements fill:none,stroke:#43a047,stroke-dasharray:5 5
```

### 声明与优先级

[syntactic.y](src/syntactic.y) 用 `%%` 分隔声明、语法规则和辅助代码。声明中的 `%union` 列出符号可以携带的值类型，`%token` 指定词法单元及其值类型。例如，`TIDENTIFIER` 携带标识符文本：

```yacc
%union {
    NBlock *block;
    NExpression *expr;
    NStatement *stmt;
    NIdentifier *ident;
    VariableList *varvec;
    ExpressionList *exprvec;
    std::string *string;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our lexical.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER "identifier" TINTEGER "integer"
%token <token> TCEQ "==" TCNE "!=" TEQUAL "="
%token <token> TLPAREN "(" TRPAREN ")" TLBRACE "{" TRBRACE "}" TCOMMA ","
%token <token> TPLUS "+" TMINUS "-" TMUL "*" TDIV "/"
%token <token> TRETURN "return" TEXTERN "extern"
%token <token> TINVALID "invalid character"
```

双引号内的文本是词法单元的别名。Bison 在错误信息中使用别名，例如 `unexpected ), expecting identifier`。

运算符的优先级也在这一部分声明，越靠后的优先级越高：

```yacc
/* Operator precedence, from lowest to highest */
%precedence TEQUAL
%nonassoc TCEQ TCNE
%left TPLUS TMINUS
%left TMUL TDIV
/* An identifier followed by "(" is always a call, never a variable
   followed by a parenthesized expression statement */
%precedence VARIABLE
%precedence TLPAREN
```

最后两行解决了一个歧义：语句之间没有分隔符，`f (x)` 既可以理解成调用 `f(x)`，也可以理解成变量 `f` 后面跟着一个表达式 `(x)`，这里规定标识符后面跟 `(` 时总是函数调用。

### 从语法规则构建 AST

规则 `program` 匹配整个程序。`stmts` 从空列表开始，每识别出一条 `stmt` 就把它加到块中。`stmt` 有四种形式：

```yacc
program:
  stmts { programBlock = $1; }
;

stmts:
  %empty { $$ = new NBlock(); }
| stmts stmt { $1->statements.emplace_back($2); $$ = $1; }
;

stmt:
  func_decl
| extern_decl
| expr { $$ = new NExpressionStatement($1); }
| TRETURN expr { $$ = new NReturnStatement($2); }
;
```

规则后面大括号中间的是动作代码，`$$` 表示规则左边符号的值，`$1`、`$2` 等依次表示右边各个符号的值。上面的动作就是在抽象语法树中生成相应的节点。再看看表示函数定义的规则：

```yacc
/* The parameter list is parsed as call_args so the parser does not have to
   decide between a declaration and a call before it sees the "{" */
func_decl:
  ident TLPAREN call_args TRPAREN block {
      VariableList params;
      if (!toParameters(*$3, params)) {
          actionError($1->line, "function parameters must be identifiers");
          delete $1; delete $3; delete $5;
          YYERROR;
      }
      $$ = new NFunctionDeclaration($1, std::move(params), $5);
      delete $3;
  }
;
```

函数调用和函数定义都以 `f(a)` 这样的形式开头。遇到 `{` 之前，语法分析器按同一条 `call_args` 规则读取参数。确认是函数定义后，`toParameters()` 才检查参数是否都是标识符：

```c++
/* Function parameters are parsed like call arguments (see func_decl);
   this checks that every argument is a plain identifier. */
static bool toParameters(ExpressionList &args, VariableList &params) {
    for (auto &arg : args) {
        if (!dynamic_cast<NIdentifier *>(arg.get())) {
            return false;
        }
    }
    for (auto &arg : args) {
        params.emplace_back(static_cast<NIdentifier *>(arg.release()));
    }
    return true;
}
```

构建时 Bison 若发现语法规则有冲突（也就是有歧义）会直接报错。

语法分析失败时，Bison 调用 `yyerror()`。它报告出错行号；如果当前位置有词法单元，还会显示对应的源码文本：

```c++
void yyerror(const char *s) {
    if (yychar == YYEOF || yychar == YYEMPTY) {
        std::fprintf(stderr, "line %d: error: %s\n", yylineno, s);
    } else {
        std::fprintf(stderr, "line %d: error: %s, near '%s'\n", yylineno, s, yytext);
    }
}
```

### 节点与所有权

[ast.h](src/ast.h) 中的所有节点都继承自 `Node`：

```c++
/* Base class of all AST nodes. Each node owns its children, so deleting the
   root block frees the whole tree. `line` is the source line used in error
   messages. */
class Node {
public:
  int line = yylineno;
  virtual ~Node() = default;
  virtual llvm::Value *codeGen(CodeGenContext &context) = 0;
};
```

函数定义节点如下：

```c++
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
```

`NFunctionDeclaration` 持有函数名、参数列表和函数体。子节点由 `std::unique_ptr` 管理，因此释放根节点就会释放整棵树。后续代码生成阶段通过虚函数 `codeGen()` 访问这些节点。

在 `src/` 目录中，以下命令生成 `syntactic.cpp`；`-d` 同时生成 `syntactic.hpp`，供词法分析器引用词法单元定义：

```bash
bison -d -o syntactic.cpp syntactic.y
```

## 目标码生成

[gen.cpp](src/gen.cpp) 为每种 AST 节点实现 `codeGen()`。这个阶段生成 LLVM IR，而不是直接生成机器码。以函数调用 `f(a, b)` 为例：

```c++
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
```

它先查找被调用的函数并检查参数个数，然后从左到右生成每个参数的代码，最后通过 LLVM 提供的 `IRBuilder` 生成调用指令。遇到调用不存在的函数、参数个数不对、使用未赋值的变量等错误时，会抛出带行号的 `CodeGenError`。生成完毕后还会用 LLVM 的 `verifyModule` 检查生成的代码是否合法。想了解更多 LLVM 的用法，可以参考 LLVM 官方的 [Kaleidoscope 教程](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html)。

### 从解析到执行

[main.cpp](src/main.cpp) 中的 `main()` 解析命令行参数并打开输入文件，然后调用 `compileAndRun()`：

```c++
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

  /* Let `extern` declarations find the runtime and the C library */
  llvm::sys::DynamicLibrary::AddSymbol("printi",
                                       reinterpret_cast<void *>(&printi));
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
  try {
    return static_cast<int>(context.runCode());
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << std::endl;
    return 1;
  }
}
```

`yyparse()` 调用 `yylex()` 读取输入并构建 AST。解析成功后，`generateCode()` 生成 IR，`-o` 决定是否写出 IR，`--no-run` 决定是否跳过执行。运行前，编译器向 JIT 注册 [runtime.cpp](src/runtime.cpp) 中的 `printi`，这样示例程序才能通过 `extern` 调用它。

## 构建

CMake 负责运行 Bison 和 Flex，确保编译词法分析器时 Bison 的头文件已经生成，最后链接 LLVM 库。在仓库根目录运行：

```bash
cmake -S . -B build
cmake --build build
```

构建完成后会在 `build/bin` 目录下生成编译器 `xy-compiler`。如果安装了多个版本的 LLVM，可以通过 `-DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm` 这样的参数指定使用哪一个，其他构建选项和运行测试的方法见附录[开发](#开发)。

## 编译测试

在仓库根目录运行 [demo.xy](demo.xy)。下面两种方式等价，分别从文件和标准输入读取源码：

```bash
./build/bin/xy-compiler demo.xy
./build/bin/xy-compiler < demo.xy
```

输出如下：

```text
11
```

要查看生成的 IR 而不执行程序，使用 `--no-run -o -`。对照输出中的 `@sum`：`alloca` 为参数分配栈空间，`store` 写入参数值，`load` 在加法前读出这两个值。

```bash
./build/bin/xy-compiler --no-run -o - demo.xy
```

```llvm
; ModuleID = 'main'
source_filename = "main"

define i64 @main() {
entry:
  %0 = call i64 @mult(i64 4, i64 5)
  %1 = call i64 @sum(i64 4, i64 5)
  %2 = sub i64 %0, %1
  %3 = call i64 @printi(i64 %2)
  ret i64 0
}

declare i64 @printi(i64)

define internal i64 @sum(i64 %a, i64 %b) {
entry:
  %a.addr = alloca i64, align 8
  %b.addr = alloca i64, align 8
  store i64 %a, ptr %a.addr, align 4
  store i64 %b, ptr %b.addr, align 4
  %a1 = load i64, ptr %a.addr, align 4
  %b2 = load i64, ptr %b.addr, align 4
  %0 = add i64 %a1, %b2
  ret i64 %0
}

define internal i64 @mult(i64 %a, i64 %b) {
entry:
  %a.addr = alloca i64, align 8
  %b.addr = alloca i64, align 8
  store i64 %a, ptr %a.addr, align 4
  store i64 %b, ptr %b.addr, align 4
  %a1 = load i64, ptr %a.addr, align 4
  %b2 = load i64, ptr %b.addr, align 4
  %0 = mul i64 %a1, %b2
  ret i64 %0
}
```

源码有错误时，编译器会报告错误所在的行并以非 0 退出码退出：

```bash
$ echo 'printi(1)' | ./build/bin/xy-compiler
line 1: error: call to undefined function 'printi'
```

## 命令行用法

```text
xy-compiler [选项] [文件]
```

不指定文件时从标准输入读取源码。编译成功后会通过 JIT 直接运行程序。

| 选项 | 说明 |
| --- | --- |
| `-f, --file <文件>` | 输入文件，等同于直接写文件名 |
| `-o, --output <文件>` | 把生成的 LLVM IR 写入文件，`-` 表示标准输出 |
| `-n, --no-run` | 只编译，不运行程序 |
| `-v, --verbose` | 在标准错误输出中打印代码生成的过程 |
| `-d, --debug` | 将语法分析过程写入标准错误 |
| `--version` | 打印编译器的版本 |
| `-h, --help` | 打印帮助 |

退出码：

| 退出码 | 含义 |
| --- | --- |
| `0` | 编译运行成功（程序没有顶层 `return`） |
| 程序的返回值 | 程序执行了顶层的 `return` 语句 |
| `1` | 源码有错误，或输入、输出文件无法打开 |
| `2` | 命令行参数错误 |

源码诊断写入标准错误，格式为 `line <行号>: error: <描述>`。

## 语言参考

### 词法

- 标识符：`[a-zA-Z_][a-zA-Z0-9_]*`，关键字 `extern` 和 `return` 除外；
- 整数：十进制数字，取值范围是 64 位有符号整数，超出范围是编译错误；没有负数字面量，也没有一元负号，负数可以写成 `0 - 1`；
- 注释：从 `//` 到行尾；
- 空格、制表符和换行（包括 Windows 的 `\r\n`）都只是分隔符。

### 语句

程序由一系列语句组成，语句之间不需要分号或换行分隔：

| 语句 | 说明 |
| --- | --- |
| `表达式` | 计算表达式的值 |
| `return 表达式` | 从当前函数返回；在顶层使用时结束程序，返回值作为进程退出码 |
| `名称(参数, ...) { 语句 ... }` | 定义函数，参数只能是标识符 |
| `extern 名称(参数, ...)` | 声明外部函数 |

### 表达式

| 表达式 | 说明 |
| --- | --- |
| `123` | 整数 |
| `x` | 读取变量 |
| `x = 表达式` | 给变量赋值，变量不存在时创建它；整个表达式的值就是所赋的值 |
| `f(参数, ...)` | 调用函数，参数从左到右求值 |
| `a + b` 等 | 二元运算，两个操作数从左到右求值 |
| `(表达式)` | 括号 |

运算符的优先级从高到低：

| 运算符 | 结合性 |
| --- | --- |
| `*` `/` | 左结合 |
| `+` `-` | 左结合 |
| `==` `!=` | 不可结合（`a == b == c` 是语法错误） |
| `=` | 右结合（`a = b = 1`） |

### 语义

- 所有的值都是 64 位有符号整数，溢出时按二进制补码回绕；
- 除法向零取整（`(0 - 7) / 2` 等于 `-3`），除以 0 是未定义行为（在 x86 上程序通常会崩溃）；
- `==` 和 `!=` 的结果是 `1` 或 `0`；
- 每个函数（包括顶层代码）都有自己独立的变量，函数内看不到顶层的变量；参数按值传递；读取从未赋值的变量是编译错误；
- 函数必须先定义或声明再调用，参数个数必须一致；在函数体内定义的函数和顶层定义的函数一样是全局的，但不能访问外层函数的变量；
- 函数的返回值是执行到的第一个 `return` 的值，没有执行 `return` 时返回 `0`，`return` 之后的语句不会执行；
- 名称 `main` 被保留给程序的入口函数，不能用作函数名；
- `extern` 声明的函数在运行时按名称在编译器进程中查找，它的参数和返回值都应该是 `long long`。编译器内置的运行时函数 `printi(val)`（见 [runtime.cpp](src/runtime.cpp)）打印 `val` 和换行，然后返回 `val`。

## 项目结构

| 路径 | 说明 |
| --- | --- |
| [src/lexical.l](src/lexical.l) | 词法规则（Flex） |
| [src/syntactic.y](src/syntactic.y) | 语法规则（Bison） |
| [src/ast.h](src/ast.h) | 抽象语法树的节点 |
| [src/gen.h](src/gen.h)、[src/gen.cpp](src/gen.cpp) | 目标码生成和 JIT 运行 |
| [src/main.cpp](src/main.cpp) | 命令行入口 |
| [src/runtime.cpp](src/runtime.cpp) | 程序可以通过 `extern` 调用的运行时函数（`printi`） |
| [demo.xy](demo.xy) | 示例程序 |
| [tests](tests) | 测试用例和测试脚本 |
| [CMakeLists.txt](CMakeLists.txt) | 构建脚本：查找依赖项、生成词法和语法分析器、编译链接、定义测试 |

## 开发

### 构建选项

| CMake 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | `Release` | 构建类型，如 `Debug` |
| `LLVM_DIR` | 自动查找 | LLVM 的 CMake 配置目录，如 `/usr/lib/llvm-18/lib/cmake/llvm` |
| `XY_BUILD_TESTS` | `ON` | 是否构建测试 |

### 测试

```bash
ctest --test-dir build --output-on-failure
```

测试用例在 [tests/cases](tests/cases) 目录下，每个 `.xy` 文件就是一个测试，在注释中写明期望的结果：

```c
// expect-stdout: 3
// expect-exit: 0
extern printi(val)
printi(1 + 2)
```

- `// expect-stdout:` 期望的一行标准输出，可以写多行，标准输出必须与之完全一致；
- `// expect-stderr:` 标准错误中应该包含的内容，不写时标准错误必须为空；
- `// expect-exit:` 期望的退出码，默认为 `0`。

新增的 `.xy` 文件会被自动加入测试（需要重新运行一次 CMake 配置）。命令行相关的测试在 [CMakeLists.txt](CMakeLists.txt) 末尾。

### 代码格式

C++ 代码使用 [.clang-format](.clang-format) 中的 LLVM 风格：

```bash
git ls-files '*.cpp' '*.h' | xargs clang-format -i                  # 格式化
git ls-files '*.cpp' '*.h' | xargs clang-format --dry-run --Werror  # 只检查，CI 中会运行
```

CI 使用 `clang-format-18`。

### 持续集成

[GitHub Actions](.github/workflows/ci.yml) 会检查代码格式，并在 Ubuntu 上分别使用 LLVM 14 和 18 构建并运行测试。

## 参考

- [Writing an Interpreter with Lex, Yacc, and Memphis](https://web.archive.org/web/20211209035751/http://memphis.compilertools.net/interpreter.html)（存档）
- [Lex & Yacc Tutorial](https://www.epaperpress.com/lexandyacc/)
- [Writing Your Own Toy Compiler Using Flex, Bison and LLVM](https://gnuu.org/2009/09/18/writing-your-own-toy-compiler/)
- [My First Language Frontend with LLVM Tutorial（Kaleidoscope）](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html)

## 许可证

[MIT](LICENSE)
