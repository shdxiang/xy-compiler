# 实现一个简单的编译器

简单的说 `编译器` 就是语言翻译器，它一般将高级语言翻译成更低级的语言，如 GCC 可将 C/C++ 语言翻译成可执行机器语言，Java 编译器可以将 Java 源代码翻译成 Java 虚拟机可以执行的字节码。

编译器如此神奇，那么它到底是如何工作的呢？本文将简单介绍编译器的原理，并实现一个简单的编译器，使它能编译我们自定义语法格式的源代码。（文中使用的源码都已上传至 [GitHub](https://github.com/shdxiang/xy-compiler) 以方便查看）。

## 自定义语法

为了简洁易懂，我们的编译器将只支持以下简单功能：

- 数据类型只支持整型，这样不需要数据类型符；

- 支持 `加`（+），`减（-）`，`乘（*）`， `除（/）` 运算

- 支持函数调用

- 支持 extern（主要是为了调用 printf 打印计算结果）

以下是我们要支持的源码实例：

```
extern printi(val)

foo(a) {
  x = a * 2
  return x + 6
}

printi(foo(5))

```

## 编译原理简介

一般编译器有以下工作步骤：

1. **词法分析（Lexical analysis）：** 此阶段的任务是从左到右一个字符一个字符地读入源程序，对构成源程序的字符流进行扫描然后根据构词规则识别 `单词（Token）`，完成这个任务的组件是 `词法分析器（Lexical analyzer，简称Lexer）`，也叫 `扫描器（Scanner）`；

1. **语法分析（Syntactic analysis，也叫 Parsing）：** 此阶段的主要任务是由 `词法分析器` 生成的单词构建 `抽象语法树（Abstract Syntax Tree ，AST）`，完成此任务的组件是 `语法分析器（Parser）`；

1. **目标码生成：** 此阶段编译器会遍历上一步生成的抽象语法树，然后为每个节点生成 `机器 / 字节码`。

编译器完成编译后，由 `链接器（Linker）` 将生成的目标文件链接成可执行文件，这一步并不是必须的，一些依赖于虚拟机运行的语言（如 Java，Erlang）就不需要链接。

## 工具简介

对应编译器工作步骤我们将使用以下工具，括号里标明了所使用的版本号：

- **[Flex（2.6.0）](https://github.com/westes/flex):** Flex 是 Lex 开源替代品，他们都是 `词法分析器` 制作工具，它可以根据我们定义的规则生成 `词法分析器` 的代码；

- **[Bison（3.0.4）](https://www.gnu.org/software/bison/)：** Bison 是 `语法分析器` 的制作工具，同样它可以根据我们定义的规则生成 `语法分析器` 的代码；

- **[LLVM（3.8.0）](http://llvm.org/)：** LLVM 是构架编译器的框架系统，我们会利用他来完成从 `抽象语法树` 生成目标码的过程。

在 ubuntu 上可以通过以下命令安装这些工具：

```
sudo apt-get install flex
sudo apt-get install bison
sudo apt-get install llvm-3.8*
```

介绍完工具，现在我们可以开始实现我们的编译器了。

## 词法分析器

前面提到 `词法分析器` 要将源程序分解成 `单词`，我们的语法格式很简单，只包括：标识符，数字，数学运算符，括号和大括号等，我们编写给 Flex 使用的规则文件 [lexical.l](https://github.com/shdxiang/xy-compiler/blob/master/src/lexical.l) 如下：

```
%{
#include <string>
#include "ast.h"
#include "syntactic.hpp"

#define SAVE_TOKEN  yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t)    (yylval.token = t)
%}

%option noyywrap

%%

[ \t\n]                 ;
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

.                       printf("Unknown token!\n"); yyterminate();

%%

```

我们来解释一下，这个文件被 2 个 `%%` 分成 3 部分，第 1 部分用 `%{` 与 `%}` 包括的是一些 C++ 代码，会被原样复制到 Flex 生成的源码文件中，还可以在指定一些选项，如我们使用了 `%option noyywrap`，也可以在这定义宏供后面使用；第 2 部分用来定义构成单词的规则，可以看到每条规都是一个 `正则表达式` 和 `动作`，很直白，就是 `词法分析器` 发现了匹配的 `单词` 后执行相应的动作代码，大部分他只要返回 `单词` 给调用者就可以了；第 3 部分可以定义一些函数，也会原样复制到生成的源码中去，这里我们留空没有使用。

现在我们可以通过调用 Flex 生成 `词法分析器` 的源码：

```
flex -o lexical.cpp lexical.l
```

生成的　lexical.cpp　里会有一个 `yylex()` 函数供　`语法分析器`　调用；你可能发现了，有些宏和变量并没有被定义（如 TEXTERN，yylval，yytext 等），其实有些是 Flex 会自动定义的内置变量（如 yytext），有些是后面 `语法分析器` 生成工具里定义的变量（如 yylval），我们后面会看到。

##　语法分析器

`语法分析器` 的作用是构建 `抽象语法树`，通俗的说 `抽象语法树` 就是将源码用树状结构来表示，每个节点都代表源码中的一种结构；对于我们要实现的语法，其语法树是很简单的，如下：

![ast.mm.svg](https://github.com/shdxiang/xy-compiler/blob/master/doc/ast.mm.svg)

现在我们使用 Bison 生成 `语法分析器` 代码，同样 Bison 需要一个规则文件，我们的规则文件 [syntactic.y](https://github.com/shdxiang/xy-compiler/blob/master/src/syntactic.y) 如下，限于篇幅，省略了某些部分，可以通过链接查看完整内容：
```
%{
    #include "ast.h"
    #include <cstdio>

...

    extern int yylex();
    void yyerror(const char *s) { std::printf("Error: %s\n", s);std::exit(1); }
%}

...

%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA

...

%%

program:
  stmts { programBlock = $1; }
        ;
...

func_decl:
  ident TLPAREN func_decl_args TRPAREN block { $$ = new NFunctionDeclaration(*$1, *$3, *$5); delete $3; }
;

...

%%

```

是不是发现和 Flex 的规则文件很像呢？确实是这样，它也是分 3 个部分组成，同样，第一部分的 C++ 代码会被复制到生成的源文件中，还可以看到这里通过以下这样的语法定义前面了 Flex 使用的宏：

```
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA
```

比较不同的是第 2 部分，不像 Flex 通过 `正则表达式` 通过定义规则，这里使用的是 `巴科斯范式(BNF: Backus-Naur Form)` 的形式定义了我们识别的语法结构。如下的语法表示函数：

```
func_decl:
  ident TLPAREN func_decl_args TRPAREN block { $$ = new NFunctionDeclaration(*$1, *$3, *$5); delete $3; }
;
```

可以看到后面大括号中间的也是 `动作` 代码，上例的动作是在 `抽象语法树` 中生成一个函数的节点。其实这部分的其他规则也是生成相应类型的节点到语法树中去。像 `NFunctionDeclaration` 这是一个我们自己定义的节点类，我们在 [ast.h](https://github.com/shdxiang/xy-compiler/blob/master/src/ast.h) 中定义了我们所要用到的节点，同样的，我们摘取一段代码如下：

```
...

class NFunctionDeclaration : public NStatement {
public:
	const NIdentifier& id;
	VariableList arguments;
	NBlock& block;
	NFunctionDeclaration(const NIdentifier& id,
			const VariableList& arguments, NBlock& block) :
		id(id), arguments(arguments), block(block) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

...

```

可以看到，它有 `标识符（id）`，`参数列表（arguments）`，`函数体（block）` 这些成员，在语法分析阶段会设置好这些成员的内容供后面的 `代码生成` 阶段使用。还可以看到有一个 `codeGen()` 虚函数，你可能猜到了，后面就是通过调用它来生成相应的目标代码。





















