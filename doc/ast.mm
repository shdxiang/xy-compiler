graph LR
   Node --- Expression
   Node --- Statement
   Expression --- Integer
   Expression --- Identifier
   Expression --- MethodCall
   Expression --- BinaryOperator
   Expression --- Block

   Statement--- ExpressionStatement
   Statement--- ExternDeclaration
   Statement--- FunctionDeclaration
