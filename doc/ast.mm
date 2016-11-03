graph TD
   Node --- NExpression
   Node --- NStatement
   NExpression --- NInteger
   NExpression --- NIdentifier
   NExpression --- NMethodCall
   NExpression --- NBinaryOperator
   NExpression --- NBlock

   NStatement--- NExpressionStatement
   NStatement--- NExternDeclaration
   NStatement--- NFunctionDeclaration
