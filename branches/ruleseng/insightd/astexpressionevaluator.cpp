#include "astexpressionevaluator.h"
#include <abstractsyntaxtree.h>
#include "astexpression.h"
#include <expressionevalexception.h>
#include <astnode.h>
#include <asttypeevaluator.h>
#include <astsourceprinter.h>
#include "symfactory.h"
#include "basetype.h"
#include "array.h"

#define checkNodeType(node, expected_type) \
    if ((node)->type != (expected_type)) { \
            exprEvalError2( \
                QString("Expected node type \"%1\", given type is \"%2\" at %3:%4") \
                    .arg(ast_node_type_to_str2(expected_type)) \
                    .arg(ast_node_type_to_str(node)) \
                    .arg(_ast->fileName()) \
                    .arg(node->start->line), \
                node); \
    }


ASTExpressionEvaluator::ASTExpressionEvaluator(ASTTypeEvaluator *eval,
                                               SymFactory *factory)
    : _ast(eval ? eval->ast() : 0), _eval(eval), _factory(factory)
{
}


ASTExpressionEvaluator::~ASTExpressionEvaluator()
{
    for (int i = 0; i < _allExpressions.size(); ++i)
        delete _allExpressions[i];
    _allExpressions.clear();
}


QString ASTExpressionEvaluator::antlrTokenToStr(
        const pANTLR3_COMMON_TOKEN tok) const
{
    // Use the AST cached version, if available
    if (_ast)
        return _ast->antlrTokenToStr(tok);
    // Otherwise use the non-caching version
    else
        return antlrStringToStr(tok->getText(tok));
}


QString ASTExpressionEvaluator::antlrStringToStr(const pANTLR3_STRING s) const
{
    // Use the AST cached version, if available
    if (_ast)
        return _ast->antlrStringToStr(s);
    // Otherwise use the non-caching version
    else
        return QString::fromAscii((const char*)s->chars, s->len);
}


template<class T>
T* ASTExpressionEvaluator::createExprNode()
{
    T* t = new T();
    _allExpressions.append(t);
    return t;
}


template<class T, class PT>
T* ASTExpressionEvaluator::createExprNode(PT param)
{
    T* t = new T(param);
    _allExpressions.append(t);
    return t;
}


template<class T, class PT1, class PT2>
T* ASTExpressionEvaluator::createExprNode(PT1 param1, PT2 param2)
{
    T* t = new T(param1, param2);
    _allExpressions.append(t);
    return t;
}


inline ASTExpression* setExprOrAddAlternative(ASTExpression *dest,
                                              ASTExpression *src)
{
    if (dest)
        dest->addAlternative(src);
    else
        dest = src;
    return dest;
}


ASTExpression* ASTExpressionEvaluator::exprOfNodeList(
        const ASTNodeList *list, const ASTNodeNodeHash& ptsTo)
{
    ASTExpression* expr = 0;
    while (list) {
        if (expr)
            expr->addAlternative(exprOfNode(list->item, ptsTo));
        else
            expr = exprOfNode(list->item, ptsTo);
        list = list->next;
    }
    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfNode(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;

    // Push current root on the recursion tracking stack, gets auto-popped later
    StackAutoPopper<typeof(_evalNodeStack)> autoPopper(&_evalNodeStack, node);
    Q_UNUSED(autoPopper);

    // Check for loops in recursive evaluation
    for (int i = 0; i < _evalNodeStack.size() - 1; ++i) {
        if (_evalNodeStack[i] == node) {
            // If we have followed inter-links before, try it without this time
            if (!ptsTo.isEmpty()) {
                ASTExpressionEvaluator other(_eval, _factory);
                ASTExpression* expr = other.exprOfNode(node, ASTNodeNodeHash());
                if (expr)
                    expr = expr->clone(_allExpressions);
                return expr;
            }
            // Otherwise raise an error
            else {
                QString msg = QString("Detected loop in recursive expression "
                                      "evaluation:\n"
                                      "File: %1\n")
                        .arg(_ast->fileName());
                int cnt = 0;
                for (int j = _evalNodeStack.size() - 1; j >= 0; --j) {
                    const ASTNode* n = _evalNodeStack[j];
                    msg += QString("%0%1. 0x%2 %3 at line %4:%5\n")
                            .arg(cnt == 0 || j == i ? "->" : "  ")
                            .arg(cnt, 4)
                            .arg((quint64)n, 0, 16)
                            .arg(ast_node_type_to_str(n), -35)
                            .arg(n->start->line)
                            .arg(n->start->charPosition);
                    ++cnt;
                }

                exprEvalError2(msg, node);
            }
        }
    }


    // Return cached value, if possible
    if (ptsTo.isEmpty() && _expressions.contains(node))
        return _expressions[node];

    // If this expression points to another expression, return that instead
    if (ptsTo.contains(node))
        return exprOfNode(ptsTo[node], ptsTo);

    ASTExpression* expr = 0;

    switch (node->type) {
    case nt_additive_expression:
    case nt_and_expression:
    case nt_equality_expression:
    case nt_exclusive_or_expression:
    case nt_inclusive_or_expression:
    case nt_logical_or_expression:
    case nt_logical_and_expression:
    case nt_multiplicative_expression:
    case nt_relational_expression:
    case nt_shift_expression:
        expr = exprOfBinaryExpr(node, ptsTo);
        break;

    case nt_assignment_expression:
        expr = exprOfAssignmentExpr(node, ptsTo);
        break;

    case nt_builtin_function_alignof:
        expr = exprOfBuiltinFuncAlignOf(node, ptsTo);
        break;

    case nt_builtin_function_choose_expr:
        expr = exprOfBuiltinFuncChooseExpr(node, ptsTo);
        break;

    case nt_builtin_function_constant_p:
        expr = exprOfBuiltinFuncConstant(node, ptsTo);
        break;

    case nt_builtin_function_expect:
        expr = exprOfBuiltinFuncExpect(node, ptsTo);
        break;

    case nt_builtin_function_extract_return_addr:
        expr = createExprNode<ASTRuntimeExpression>();
        break;

    case nt_builtin_function_object_size:
        expr = exprOfBuiltinFuncObjectSize(node, ptsTo);
        break;

    case nt_builtin_function_offsetof:
        expr = exprOfBuiltinFuncOffsetOf(node, ptsTo);
        break;

    case nt_builtin_function_prefetch:
        expr = createExprNode<ASTVoidExpression>();
        break;

    case nt_builtin_function_return_address:
        expr = createExprNode<ASTRuntimeExpression>();
        break;

    case nt_builtin_function_sizeof:
        expr = exprOfBuiltinFuncSizeof(node, ptsTo);
        break;

    case nt_builtin_function_types_compatible_p:
        expr = exprOfBuiltinFuncTypesCompatible(node, ptsTo);
        break;

    case nt_builtin_function_va_arg:
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_end:
    case nt_builtin_function_va_start:
        expr = createExprNode<ASTRuntimeExpression>();
        break;

    case nt_cast_expression:
        if (node->u.cast_expression.type_name) {
            assert(node->u.cast_expression.cast_expression != 0);
            return exprOfNode(node->u.cast_expression.cast_expression, ptsTo);
        }
        else
            return exprOfNode(node->u.cast_expression.unary_expression, ptsTo);
        break;

    case nt_conditional_expression:
        expr = exprOfConditionalExpr(node, ptsTo);
        break;

    case nt_constant_expression:
        expr = exprOfNode(node->u.constant_expression.conditional_expression, ptsTo);
        break;

    case nt_constant_char:
    case nt_constant_int:
    case nt_constant_float:
        expr = exprOfConstant(node, ptsTo);
        break;

    case nt_expression_statement:
        if (node->u.expression_statement.expression) {
            // Find last expression
            const ASTNodeList* list = node->u.expression_statement.expression;
            while (list && list->next)
                list = list->next;
            expr = list ? exprOfNode(list->item, ptsTo) : 0;
        }
        break;

    case nt_initializer:
        if (node->u.initializer.assignment_expression)
            expr = exprOfNode(node->u.initializer.assignment_expression, ptsTo);
        break;

    case nt_jump_statement_return:
        if (node->u.jump_statement.initializer)
            expr = exprOfNode(node->u.jump_statement.initializer, ptsTo);
        break;

    case nt_lvalue:
        // Could be an lvalue cast
        if (node->u.lvalue.lvalue)
            expr = exprOfNode(node->u.lvalue.lvalue, ptsTo);
        else
            expr = exprOfNode(node->u.lvalue.unary_expression, ptsTo);
        break;

    case nt_postfix_expression:
        expr = exprOfPostfixExpr(node, ptsTo);
        break;

    case nt_primary_expression:
        expr = exprOfPrimaryExpr(node, ptsTo);
        break;

    case nt_unary_expression:
    case nt_unary_expression_builtin:
    case nt_unary_expression_dec:
    case nt_unary_expression_inc:
    case nt_unary_expression_op:
        expr = exprOfUnaryExpr(node, ptsTo);
        break;

    default:
        exprEvalError2(QString("Unexpexted node type: %1 at %2:%3:%4")
                       .arg(ast_node_type_to_str(node))
                       .arg(_ast ? _ast->fileName() : QString())
                       .arg(node->start->line)
                       .arg(node->start->charPosition),
                       node);
    }

    if (!expr) {
        exprEvalError2(QString("Failed to evaluate node %1 at %2:%3:%4")
                       .arg(ast_node_type_to_str(node))
                       .arg(_ast->fileName())
                       .arg(node->start->line)
                       .arg(node->start->charPosition),
                       node);
    }

    if (ptsTo.isEmpty())
        _expressions[node] = expr;
    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfAssignmentExpr(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_assignment_expression);

    if (node->u.assignment_expression.assignment_expression)
        return exprOfNode(node->u.assignment_expression.assignment_expression, ptsTo);
    else if (node->u.assignment_expression.lvalue)
        return exprOfNode(node->u.assignment_expression.lvalue, ptsTo);
    else
        return exprOfNode(node->u.assignment_expression.conditional_expression, ptsTo);
}


ASTExpression* ASTExpressionEvaluator::exprOfBinaryExpr(
        const ASTNode *node,const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;

    if (!node->u.binary_expression.right)
        return exprOfNode(node->u.binary_expression.left, ptsTo);

    ExpressionType exprType;
    QString op;

    switch (node->type) {
    case nt_logical_or_expression:
        exprType = etLogicalOr;
        break;
    case nt_logical_and_expression:
        exprType = etLogicalAnd;
        break;
    case nt_inclusive_or_expression:
        exprType = etInclusiveOr;
        break;
    case nt_exclusive_or_expression:
        exprType = etExclusiveOr;
        break;
    case nt_and_expression:
        exprType = etAnd;
        break;
    case nt_equality_expression:
        op = antlrTokenToStr(node->u.binary_expression.op);
        if (op == "==")
            exprType = etEquality;
        else
            exprType = etUnequality;
        break;
    case nt_relational_expression:
        op = antlrTokenToStr(node->u.binary_expression.op);
        if (op == ">")
            exprType = etRelationalGT;
        else if (op == ">=")
            exprType = etRelationalGE;
        else if (op == "<")
            exprType = etRelationalLT;
        else
            exprType = etRelationalLE;
        break;
    case nt_shift_expression:
        op = antlrTokenToStr(node->u.binary_expression.op);
        if (op == "<<")
            exprType = etShiftLeft;
        else
            exprType = etShiftRight;
        break;
    case nt_additive_expression:
        op = antlrTokenToStr(node->u.binary_expression.op);
        if (op == "+")
            exprType = etAdditivePlus;
        else
            exprType = etAdditiveMinus;
        break;
    case nt_multiplicative_expression:
        op = antlrTokenToStr(node->u.binary_expression.op);
        if (op == "*")
            exprType = etMultiplicativeMult;
        else if (op == "/")
            exprType = etMultiplicativeDiv;
        else
            exprType = etMultiplicativeMod;
        break;
    default:
        exprEvalError2(QString("Type \"%1\" represents no binary expression at %2:%3:%4")
                       .arg(ast_node_type_to_str(node))
                       .arg(_ast->fileName())
                       .arg(node->start->line)
                       .arg(node->start->charPosition),
                       node);
    }

    ASTBinaryExpression *expr = createExprNode<ASTBinaryExpression>(exprType);
    expr->setLeft(exprOfNode(node->u.binary_expression.left, ptsTo));
    expr->setRight(exprOfNode(node->u.binary_expression.right, ptsTo));
    return expr;
}

/*
  The keyword __alignof__ allows you to inquire about how an object is aligned,
  or the minimum alignment usually required by a type. Its syntax is just like
  sizeof.

  For example, if the target machine requires a double value to be aligned on an
  8-byte boundary, then __alignof__ (double) is 8. This is true on many RISC
  machines. On more traditional machine designs, __alignof__ (double) is 4 or
  even 2.

  Some machines never actually require alignment; they allow reference to any
  data type even at an odd address. For these machines, __alignof__ reports the
  smallest alignment that GCC will give the data type, usually as mandated by
  the target ABI.

  If the operand of __alignof__ is an lvalue rather than a type, its value is
  the required alignment for its type, taking into account any minimum alignment
  specified with GCC's __attribute__ extension (see Variable Attributes). For
  example, after this declaration:

     struct foo { int x; char y; } foo1;

  the value of __alignof__ (foo1.y) is 1, even though its actual alignment is
  probably 2 or 4, the same as __alignof__ (int).

  It is an error to ask for the alignment of an incomplete type.

  Source: http://gcc.gnu.org/onlinedocs/gcc/Alignment.html
 */
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncAlignOf(
        const ASTNode *node,  const ASTNodeNodeHash& /*ptsTo*/)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_alignof);

    const ASTNode* n = node->u.builtin_function_alignof.unary_expression ?
                node->u.builtin_function_alignof.unary_expression :
                node->u.builtin_function_alignof.type_name;
    const ASTType* t = _eval->typeofNode(n);
    quint64 value = 0;

    // Alignment ignores arrays and top-level qualifiers
    while (t && t->type() & (rtArray|rtConst|rtVolatile))
        t = t->next();

    // For numeric, function, and pointer types, __alignof__(t) == sizeof(t)
    if (t->type() & (NumericTypes|FunctionTypes|rtPointer|rtVoid))
        value = sizeofType(t);
    // For struct, alignment depends on the size
    else if (t->type() & StructOrUnion) {
        value = sizeofType(t);
        if (value >= (unsigned int)_eval->sizeofLong())
            value = (unsigned int)_eval->sizeofLong();
        else if (value >= 4)
            value = 4;
        else if (value >= 2)
            value = 2;
        else
            value = 1;
    }

    return createExprNode<ASTConstantExpression>(
                _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                value);
}

/*
  You can use the built-in function __builtin_choose_expr to evaluate code
  depending on the value of a constant expression. This built-in function
  returns exp1 if const_exp, which is a constant expression that must be able to
  be determined at compile time, is nonzero. Otherwise it returns 0.

  This built-in function is analogous to the `? :' operator in C, except that
  the expression returned has its type unaltered by promotion rules. Also, the
  built-in function does not evaluate the expression that was not chosen. For
  example, if const_exp evaluates to true, exp2 is not evaluated even if it has
  side-effects.

  This built-in function can return an lvalue if the chosen argument is an
  lvalue.

  If exp1 is returned, the return type is the same as exp1's type. Similarly, if
  exp2 is returned, its return type is the same as exp2.

  Example:

          #define foo(x)                                                    \
            __builtin_choose_expr (                                         \
              __builtin_types_compatible_p (typeof (x), double),            \
              foo_double (x),                                               \
              __builtin_choose_expr (                                       \
                __builtin_types_compatible_p (typeof (x), float),           \
                foo_float (x),                                              \
                / * The void expression results in a compile-time error  \
                   when assigning the result to something.  * /          \
                (void)0))

  Source: http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Other-Builtins.html
 */
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncChooseExpr(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_choose_expr);

    ASTExpression *ret = 0, *ae1 = 0, *ae2;

    for (ASTExpression *expr = exprOfNode(
                node->u.builtin_function_choose_expr.constant_expression,
                ptsTo);
         expr;
         expr = expr->alternative())
    {
        ExpressionResult res = expr->result();
        // Is the result valid?
        if (res.resultType == erConstant) {
            const ASTNode *n = res.result.i64 ?
                        node->u.builtin_function_choose_expr.assignment_expression1 :
                        node->u.builtin_function_choose_expr.assignment_expression2;
            ae1 = exprOfNode(n, ptsTo);
            ret = setExprOrAddAlternative(ret, ae1);
        }
        // If not, add both alternatives
        else {
            ae1 = exprOfNode(
                        node->u.builtin_function_choose_expr.assignment_expression1,
                        ptsTo);
            ae2 = exprOfNode(
                        node->u.builtin_function_choose_expr.assignment_expression2,
                        ptsTo);
            ret = setExprOrAddAlternative(ret, ae1);
            ret = setExprOrAddAlternative(ret, ae2);
        }
    }

    return ret;
}

/*
  Built-in Function: size_t __builtin_object_size (void * ptr, int type)

  is a built-in construct that returns a constant number of bytes from ptr to
  the end of the object ptr pointer points to (if known at compile time).
  __builtin_object_size never evaluates its arguments for side-effects. If there
  are any side-effects in them, it returns (size_t) -1 for type 0 or 1 and
  (size_t) 0 for type 2 or 3. If there are multiple objects ptr can point to and
  all of them are known at compile time, the returned number is the maximum of
  remaining byte counts in those objects if type & 2 is 0 and minimum if
  nonzero. If it is not possible to determine which objects ptr points to at
  compile time, __builtin_object_size should return (size_t) -1 for type 0 or 1
  and (size_t) 0 for type 2 or 3.

  type is an integer constant from 0 to 3. If the least significant bit is
  clear, objects are whole variables, if it is set, a closest surrounding
  subobject is considered the object a pointer points to. The second bit
  determines if maximum or minimum of remaining bytes is computed.

  Source: http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Object-Size-Checking.html
 */
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncObjectSize(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_object_size);

    ASTExpression *expr = exprOfNode(node->u.builtin_function_object_size.constant, ptsTo);
    ExpressionResult res = expr->result();
    assert(res.resultType == erConstant);

    ASTConstantExpression* ret =
            createExprNode<ASTConstantExpression>(
                _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                0ULL);
    // If the maximum remaining bytes are requested, return (size_t) -1 instead
    // of 0.
    if (res.result.ui64 & 2)
        ret->setValue(ret->result().size, 0ULL);
    else
        ret->setValue(ret->result().size, -1ULL);
    return ret;
}


/*
  You can use the built-in function __builtin_constant_p to determine if a value
  is known to be constant at compile-time and hence that GCC can perform
  constant-folding on expressions involving that value. The argument of
  the function is the value to test. The function returns the integer 1 if the
  argument is known to be a compile-time constant and 0 if it is not known to be
  a compile-time constant. A return of 0 does not indicate that the value is not
  a constant, but merely that GCC cannot prove it is a constant with the
  specified value of the -O option.

  You would typically use this function in an embedded application where memory
  was a critical resource. If you have some complex calculation, you may want it
  to be folded if it involves constants, but need to call a function if it does
  not. For example:

          #define Scale_Value(X)      \
            (__builtin_constant_p (X) \
            ? ((X) * SCALE + OFFSET) : Scale (X))

  Source: http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Other-Builtins.html
 */
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncConstant(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_constant_p);

    ASTExpression *expr = exprOfNode(
                node->u.builtin_function_constant_p.unary_expression, ptsTo);
    // We can only approximate GCC's constant value decision here
    return expr->resultType() == erConstant ?
                createExprNode<ASTConstantExpression>(esInt32, 1ULL) :
                createExprNode<ASTConstantExpression>(esInt32, 0ULL);

}

/*
  You may use __builtin_expect to provide the compiler with branch prediction
  information. In general, you should prefer to use actual profile feedback for
  this (-fprofile-arcs), as programmers are notoriously bad at predicting how
  their programs actually perform. However, there are applications in which this
  data is hard to collect.

  The return value is the value of exp, which should be an integral expression.
  The semantics of the built-in are that it is expected that exp == c. For
  example:

          if (__builtin_expect (x, 0))
            foo ();

  would indicate that we do not expect to call foo, since we expect x to be
  zero. Since you are limited to integral expressions for exp, you should use
  constructions such as

          if (__builtin_expect (ptr != NULL, 1))
            error ();

  when testing pointer or floating-point values.

  Source: http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Other-Builtins.html
*/
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncExpect(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_expect);

    ASTExpression* expr =
            exprOfNode(node->u.builtin_function_expect.assignment_expression, ptsTo);
    /// @todo Return type is long
//    if (_eval->sizeofLong() == 4) {
//    }
//    else {
//    }

    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncOffsetOf(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_offsetof);

    ASTType* type = _eval->typeofNode(node->u.builtin_function_offsetof.type_name);
    if (!type || !(type->type() & StructOrUnion)) {
        ASTSourcePrinter printer(_ast);
        exprEvalError2(QString("Cannot find struct/union definition for \"%1\" "
                              "at %2:%3:%4")
                      .arg(printer.toString(
                               node->u.builtin_function_offsetof.postfix_expression)
                           .trimmed())
                      .arg(_ast->fileName())
                      .arg(node->start->line)
                      .arg(node->start->charPosition),
                       node);
    }

    ASTExpression* expr = 0;
    BaseTypeList bt_list = _factory->findBaseTypesForAstType(type, _eval).typesNonPtr;

    // Try each type of the list in turn
    int structTypes = 0;
    for (int i = 0; i < bt_list.size(); ++i) {
        if (bt_list[i]->type() & StructOrUnion) {
            structTypes++;
            bool exceptions = (i + 1 == bt_list.size());
            expr = exprOfBuiltinFuncOffsetOfSingle(node, bt_list[i], type,
                                                   ptsTo, exceptions);
            if (expr)
                return expr;
        }
    }


    ASTSourcePrinter printer(_ast);
    exprEvalError3(QString("Failed to evaluate offsetof() expression for \"%1\" "
                          "at %2:%3:%4")
                  .arg(printer.toString(
                           node->u.builtin_function_offsetof.postfix_expression)
                       .trimmed())
                  .arg(_ast->fileName())
                  .arg(node->start->line)
                  .arg(node->start->charPosition),
                   node,
                   structTypes ? ExpressionEvalException::ecUndefined
                               : ExpressionEvalException::ecTypeNotFound);

    return 0;
}


ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncOffsetOfSingle(
        const ASTNode *node, const BaseType* bt, const ASTType* type,
        const ASTNodeNodeHash& ptsTo, bool exceptions)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_offsetof);

    quint64 offset = 0;
    const ASTNode *arrayIndexExpr = 0;
    const ASTNode *pfe = node->u.builtin_function_offsetof.postfix_expression;
    const ASTNode *pre = pfe->u.postfix_expression.primary_expression;
    const ASTNodeList *pfesl =
            pfe->u.postfix_expression.postfix_expression_suffix_list;
    QString name = antlrTokenToStr(pre->u.primary_expression.identifier);

    const Structured* s = 0;
    const StructuredMember* m = 0;

    do {
        // Offset of array operator to member
        if (arrayIndexExpr) {
            // We should have found a member already
            assert(m != 0);
            assert(bt != 0);
            // The type of bt should be rtArray
            const Array* a = dynamic_cast<const Array*>(bt);
            if (!a) {
                if (!exceptions)
                    return 0;
                exprEvalError2(QString("Type \"%1\" is not an array at %2:%3:%4")
                              .arg(bt ? bt->prettyName() : QString("NULL"))
                              .arg(_ast->fileName())
                              .arg(pfesl ?
                                       pfesl->item->start->line :
                                       pfe->start->line)
                              .arg(pfesl ?
                                       pfesl->item->start->charPosition :
                                       pfe->start->charPosition),
                               node);
            }
            // The new BaseType is the array's referencing type
            bt = a->refTypeDeep(BaseType::trLexical);

            ASTExpression* expr = exprOfNode(arrayIndexExpr, ptsTo);
            ExpressionResult index = expr->result();
            // We should find at least one constant expression
            while (expr->hasAlternative() && index.resultType != erConstant) {
                expr = expr->alternative();
                index = expr->result();
            }
            // The array operator is allowed to have a runtime expression
            if (index.resultType != erConstant) {
                if (expr->type() == etRuntimeDependent ||
                    expr->type() == etUndefined)
                    return expr;
                else
                    return createExprNode<ASTRuntimeExpression>();
            }
            // Add array offset to total offset
            offset += index.result.ui64 * bt->size();
            // Dereference
        }
        // Offset of member by name
        else {
            s = dynamic_cast<const Structured*>(bt);
            if (!s) {
                if (!exceptions)
                    return 0;
                exprEvalError2(QString("Cannot find type struct/union BaseType "
                                      "for \"%1\" at %2:%3:%4")
                              .arg(type->toString())
                              .arg(_ast->fileName())
                              .arg(pfesl ?
                                       pfesl->item->start->line :
                                       pfe->start->line)
                              .arg(pfesl ?
                                       pfesl->item->start->charPosition :
                                       pfe->start->charPosition),
                               node);
            }

            m = s->member(name);
            if (!m) {
                if (!exceptions)
                    return 0;
                exprEvalError2(QString("Cannot find member \"%1\" in %2 at "
                                      "%3:%4:%5")
                              .arg(name)
                              .arg(s->prettyName())
                              .arg(_ast->fileName())
                              .arg(pfesl ?
                                       pfesl->item->start->line :
                                       pfe->start->line)
                              .arg(pfesl ?
                                       pfesl->item->start->charPosition :
                                       pfe->start->charPosition),
                               node);
            }
            // Add the member's offset to total offset
            offset += m->offset();
            bt = m->refTypeDeep(BaseType::trLexical);
        }

        // Advance to next item in list, or end the loop
        if (pfesl) {
            if (pfesl->item->type == nt_postfix_expression_dot) {
                name = antlrTokenToStr(pfesl->item
                                       ->u.postfix_expression_suffix.identifier);
                arrayIndexExpr = 0;
            }
            else if (pfesl->item->type == nt_postfix_expression_brackets) {
                name.clear();
                arrayIndexExpr = pfesl->item
                        ->u.postfix_expression_suffix.expression
                        ->item;
            }
            else {
                if (!exceptions)
                    return 0;
                exprEvalError2(QString("Unexpected postfix expression suffix "
                                      "type: %1 at %2:%3:%4")
                              .arg(ast_node_type_to_str(pfesl->item))
                              .arg(_ast->fileName())
                              .arg(node->start->line)
                              .arg(node->start->charPosition),
                               node);
            }
            pfesl = pfesl->next;
        }
        else
            break;

    } while (true);

    // Return the result
    return createExprNode<ASTConstantExpression>(
                _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                offset);
}


ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncSizeof(
        const ASTNode *node, const ASTNodeNodeHash& /*ptsTo*/)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_sizeof);

    const ASTNode* n = node->u.builtin_function_sizeof.unary_expression ?
                node->u.builtin_function_sizeof.unary_expression :
                node->u.builtin_function_sizeof.type_name;

    ASTType* type = _eval->typeofNode(n);
    assert(type != 0);
    // For pointers, we can make this quick
    if (type->type() & (rtPointer|rtFuncPointer|rtFunction))
        return createExprNode<ASTConstantExpression>(
                    _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                    (quint64)_eval->sizeofLong());
    // Return alternatives for all sizes
    FoundBaseTypes found = _factory->findBaseTypesForAstType(type, _eval);
    BaseTypeList list = found.typesNonPtr;
    int arrayLen = -1;
    // Find array definitions of type
    for (ASTType* tmp = type; tmp && tmp != found.astTypeNonPtr;
         tmp = tmp->next())
    {
        // Consider length of arrays
        if (tmp->type() == rtArray && tmp->arraySize() >= 0) {
            if (arrayLen < 0)
                arrayLen = tmp->arraySize();
            else
                arrayLen *= tmp->arraySize();
        }
        // No array type or type not evaluatable: reset
        else
            arrayLen = -1;
    }

    QSet<quint32> sizes;
    ASTExpression *ret = 0;
    for (int i = 0; i < list.size(); ++i) {
        quint32 size = list[i]->size();
        if (arrayLen >= 0)
            size *= arrayLen;
        // Add each size greater zero only once
        if (size > 0 && !sizes.contains(size)) {
            ASTExpression *expr =
                    createExprNode<ASTConstantExpression>(
                        _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                        (quint64)size);
            ret = setExprOrAddAlternative(ret, expr);
            sizes.insert(size);
        }
    }

    // If we did not find that type, create an invalid expression
    if (!ret)
        ret = createExprNode<ASTUndefinedExpression>();

    return ret;
}

/*
  You can use the built-in function __builtin_types_compatible_p to determine
  whether two types are the same.

  This built-in function returns 1 if the unqualified versions of the types
  type1 and type2 (which are types, not expressions) are compatible, 0
  otherwise. The result of this built-in function can be used in integer
  constant expressions.

  This built-in function ignores top level qualifiers (e.g., const, volatile).
  For example, int is equivalent to const int.

  The type int[] and int[5] are compatible. On the other hand, int and char *
  are not compatible, even if the size of their types, on the particular
  architecture are the same. Also, the amount of pointer indirection is taken
  into account when determining similarity. Consequently, short * is not similar
  to short **. Furthermore, two types that are typedefed are considered
  compatible if their underlying types are compatible.

  An enum type is not considered to be compatible with another enum type even if
  both are compatible with the same integer type; this is what the C standard
  specifies. For example, enum {foo, bar} is not similar to enum {hot, dog}.

  You would typically use this function in code whose execution varies depending
  on the arguments' types. For example:

              #define foo(x)                                                  \
                ({                                                           \
                  typeof (x) tmp = (x);                                       \
                  if (__builtin_types_compatible_p (typeof (x), long double)) \
                    tmp = foo_long_double (tmp);                              \
                  else if (__builtin_types_compatible_p (typeof (x), double)) \
                    tmp = foo_double (tmp);                                   \
                  else if (__builtin_types_compatible_p (typeof (x), float))  \
                    tmp = foo_float (tmp);                                    \
                  else                                                        \
                    abort ();                                                 \
                  tmp;                                                        \
                })

  Source: http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Other-Builtins.html
 */
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncTypesCompatible(
        const ASTNode *node, const ASTNodeNodeHash& /*ptsTo*/)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_types_compatible_p);

    ASTType *t1 = _eval->typeofNode(
                node->u.builtin_function_types_compatible_p.type_name1);
    ASTType *t2 = _eval->typeofNode(
                node->u.builtin_function_types_compatible_p.type_name2);
    // Defer equality decision to ASTType for now. This might not be exactly
    // what GCC decides.
    /// @todo More acurate type equality decision
    return t1->equalTo(t2, true) ?
                createExprNode<ASTConstantExpression>(esInt32, 1ULL) :
                createExprNode<ASTConstantExpression>(esInt32, 0ULL);
}


ASTExpression* ASTExpressionEvaluator::exprOfConditionalExpr(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_conditional_expression);

    if (node->u.conditional_expression.conditional_expression) {
        // Evaluate condition
        ASTExpression* ret = 0, *tmp = 0;

        for (ASTExpression* expr = exprOfNode(
                    node->u.conditional_expression.logical_or_expression,
                 ptsTo);
             expr;
             expr = expr->alternative())
        {
            // For constant conditions we can choose the correct path
            if (expr->resultType() == erConstant) {
                if (expr->result().result.i64)
                    tmp = exprOfNodeList(
                                node->u.conditional_expression.expression,
                                ptsTo);
                else
                    tmp = exprOfNode(
                                node->u.conditional_expression.conditional_expression,
                                ptsTo);
                ret = setExprOrAddAlternative(ret, tmp);
            }
            // Otherwise add both possibilities as alternatives
            else {
                tmp = exprOfNodeList(
                            node->u.conditional_expression.expression, ptsTo);
                ret = setExprOrAddAlternative(ret, tmp);
                tmp = exprOfNode(
                            node->u.conditional_expression.conditional_expression,
                            ptsTo);
                ret = setExprOrAddAlternative(ret, tmp);
            }
        }

        return ret;
    }
    else
        return exprOfNode(node->u.conditional_expression.logical_or_expression, ptsTo);
}


ASTExpression* ASTExpressionEvaluator::exprOfConstant(
        const ASTNode *node, const ASTNodeNodeHash& /*ptsTo*/)
{
    if (!node)
        return 0;

    bool ok = false;
    ExpressionResultSize size = esUndefined;
    ASTConstantExpression *expr = 0;

    switch (node->type) {
    case nt_constant_int: {
        quint64 value;
        RealType rt = _eval->realTypeOfConstInt(node, &value);
        size = realTypeToResultSize(rt);
        return createExprNode<ASTConstantExpression>(size, value);
    }

    case nt_constant_char: {
        QString s = antlrTokenToStr(node->u.constant.literal);
        // Keep it simple for now
        unsigned char c = 0; // unsigned to avoid sign expansion
        // Single letter
        if (s.size() == 3) {
            c = s[1].toLatin1();
            expr = createExprNode<ASTConstantExpression>(esInt8, (quint64)c);
        }
        // Simple escape sequence
        else if (s.size() >= 4 && s[1] == QChar('\\')) {
            switch (s[2].toLatin1()) {
            case 'a':  c = '\a'; break;
            case 'b':  c = '\b'; break;
            case 't':  c = '\t'; break;
            case 'n':  c = '\n'; break;
            case 'f':  c = '\f'; break;
            case 'r':  c = '\r'; break;
            case 'e':  c = '\e'; break;
            case 'E':  c = '\E'; break;
            case 'v':  c = '\v'; break;
            case '\"': c = '\"'; break;
            case '\'': c = '\''; break;
            case '\\': c = '\\'; break;
            }
            if (c)
                expr = createExprNode<ASTConstantExpression>(esInt8, (quint64)c);
            else {
                // Hex escape sequence
                if (s[2] == QChar('x')) {
                    c = s.mid(3, s.length() - 4).toULong(&ok, 16);
                    expr = createExprNode<ASTConstantExpression>(esInt8,
                                                                 (quint64)c);
                }
//                else if (s[2] == QChar('u') || s[2] == QChar('U')) {
//                    unsigned int i;
//                    i = s.mid(3, s.length() - 4).toUInt(&ok, 16);
//                    expr = createExprNode<ASTConstantExpression>(esInt32,
//                                                                 (quint64)i);
//                }
                else if (s[2].category() == QChar::Number_DecimalDigit) {
                    c = s.mid(2, s.length() - 3).toUInt(&ok, 8);
                    expr = createExprNode<ASTConstantExpression>(esInt8,
                                                                 (quint64)c);
                }
            }
        }

        if (!expr)
            exprEvalError2(QString("Unsupported escape sequence: %1 at %2:%3:%4")
                          .arg(s)
                          .arg(_ast->fileName())
                          .arg(node->start->line)
                          .arg(node->start->charPosition),
                           node);

        return expr;
    }

    case nt_constant_float: {
        double value;
        RealType rt = _eval->realTypeOfConstFloat(node, &value);
        size = realTypeToResultSize(rt);
        if (rt == rtFloat)
            return createExprNode<ASTConstantExpression>(size, (float)value);
        else
            return createExprNode<ASTConstantExpression>(size, value);
    }

    default:
        exprEvalError2(QString("Unexpected constant value type: %1 at %2:%3:%4")
                      .arg(ast_node_type_to_str(node))
                      .arg(_ast->fileName())
                      .arg(node->start->line)
                      .arg(node->start->charPosition),
                       node);
    }

    return 0;
}


ASTExpression* ASTExpressionEvaluator::exprOfPostfixExpr(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_postfix_expression);

    ASTExpression* expr =
            exprOfNode(node->u.postfix_expression.primary_expression, ptsTo);

    const ASTNodeList* suffixList =
            node->u.postfix_expression.postfix_expression_suffix_list;

    // Append postfix expression suffixes
    if (expr && suffixList) {
        ASTVariableExpression* var =
                dynamic_cast<ASTVariableExpression*>(expr);
        bool result;

        // May also be a ASTRuntimeExpression
        if (var)
            result = appendPostfixExpressionSuffixes(suffixList, var);
        else
            return expr;

        if (!result)
            expr = createExprNode<ASTRuntimeExpression>();
    }

    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfPrimaryExpr(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_primary_expression);

    ASTExpression* expr = 0;

    if (node->u.primary_expression.expression)
        expr = exprOfNode(node->u.primary_expression.expression->item, ptsTo);
    else if (node->u.primary_expression.identifier) {
        const ASTSymbol* sym = _eval->findSymbolOfPrimaryExpression(node);
        // Return a constant expression for an enumerator
        if (sym->type() == stEnumerator) {
            if (!_factory->enumsByName().contains(sym->name())) {
//                exprEvalError2(QString("Cannot find enumerator \"%1\" at %2:%3:%4")
//                              .arg(sym->name())
//                              .arg(_ast->fileName())
//                              .arg(node->start->line)
//                              .arg(node->start->charPosition));

                // If no type using that enumerator is declared, it won't be
                // include in the debugging symbols
                return createExprNode<ASTUndefinedExpression>();
            }
            IntEnumPair iep = _factory->enumsByName().value(sym->name());
            return createExprNode<ASTEnumeratorExpression>(iep.first, sym);
        }
        // Otherwise return a variable
        const ASTType* type = _eval->typeofSymbol(sym);
        FoundBaseTypes found = _factory->findBaseTypesForAstType(type, _eval);
        const BaseType* bt = found.types.isEmpty() ?
                    0 : found.types.first();
        expr = createExprNode<ASTVariableExpression>(bt);
    }
    else if (node->u.primary_expression.constant)
        expr = exprOfNode(node->u.primary_expression.constant, ptsTo);
    else if (node->u.primary_expression.compound_braces_statement) {
        // The last expression is the result
        const ASTNode* cbs = node->u.primary_expression.compound_braces_statement;
        const ASTNodeList* statements =
                cbs->u.compound_braces_statement.declaration_or_statement_list;
        while (statements && statements->next)
            statements = statements->next;
        expr = statements ? exprOfNode(statements->item, ptsTo) : 0;
    }
    else
        exprEvalError2(QString("Unexpected primary expression at %2:%3:%4")
                      .arg(_ast->fileName())
                      .arg(node->start->line)
                      .arg(node->start->charPosition),
                       node);

    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfUnaryExpr(
        const ASTNode *node, const ASTNodeNodeHash& ptsTo)
{
    if (!node)
        return 0;

    ASTUnaryExpression* ue = 0;
    ASTExpression* child = 0;

    switch (node->type) {
    case nt_unary_expression:
        return exprOfNode(node->u.unary_expression.postfix_expression, ptsTo);

    case nt_unary_expression_builtin:
        return exprOfNode(node->u.unary_expression.builtin_function, ptsTo);

    case nt_unary_expression_dec:
        ue = createExprNode<ASTUnaryExpression>(etUnaryDec);
        child = exprOfNode(node->u.unary_expression.unary_expression, ptsTo);
        break;

    case nt_unary_expression_inc:
        ue = createExprNode<ASTUnaryExpression>(etUnaryInc);
        child = exprOfNode(node->u.unary_expression.unary_expression, ptsTo);
        break;

    case nt_unary_expression_op: {
        QString op = antlrTokenToStr(node->u.unary_expression.unary_operator);
        child = exprOfNode(node->u.unary_expression.cast_expression, ptsTo);
        ASTVariableExpression* var =
                dynamic_cast<ASTVariableExpression*>(child);

        // For address operators, the child must be a variable expression
        if (op == "&" || op == "&&") {
            // Can also be a runtime expression
            if (var) {
                // See if this address operator was skipped
                ASTType* type = _eval->typeofNode(node);
                if (!type->ampersandSkipped())
                    var->appendTransformation(ttAddress);
                if (op == "&&")
                    var->appendTransformation(ttAddress);
            }
        }
        else if (op == "*") {
            // The child is most likely a variable
            if (var)
                var->appendTransformation(ttDereference);
            else
                ue = createExprNode<ASTUnaryExpression>(etUnaryStar);
        }
        else if (op == "+")
            return child;
        else if (op == "-")
            ue = createExprNode<ASTUnaryExpression>(etUnaryMinus);
        else if (op == "~")
            ue = createExprNode<ASTUnaryExpression>(etUnaryInv);
        else if (op == "!")
            ue = createExprNode<ASTUnaryExpression>(etUnaryNot);
        else
            exprEvalError2(QString("Unhandled unary operator: \"%1\" at %2:%3:%4")
                           .arg(op)
                           .arg(_ast->fileName())
                           .arg(node->start->line)
                           .arg(node->start->charPosition),
                           node);
        break;
    }

    default:
        exprEvalError2(QString("Type \"%1\" represents no unary expression at %2:%3:%4")
                       .arg(ast_node_type_to_str(node))
                       .arg(_ast->fileName())
                       .arg(node->start->line)
                       .arg(node->start->charPosition),
                       node);
    }

    if (ue)
        ue->setChild(child);
    return ue ? ue : child;
}


unsigned int ASTExpressionEvaluator::sizeofType(const ASTType *type)
{
    while (type && (type->type() & (rtConst|rtVolatile)))
        type = type->next();

    if (!type)
        return 0;

    const ASTNode* node = 0;

    switch (type->type()) {
    case rtInt8:
    case rtUInt8:
    case rtBool8:
        return 1;

	case rtInt16:
	case rtUInt16:
	case rtBool16:
		return 2;

	case rtInt32:
	case rtUInt32:
	case rtBool32:
		return 4;

	case rtInt64:
	case rtUInt64:
	case rtBool64:
		return 8;

	case rtFloat:
		return 4;

	case rtDouble:
		return 8;

	case rtPointer:
		return _eval->sizeofPointer();

	/// @todo
//	case rtArray:
//		break;

	case rtEnum:
		return 4;

	case rtStruct:
	case rtUnion: {
		// Find type in the factory
		FoundBaseTypes found =
				_factory->findBaseTypesForAstType(type, _eval);
		if (!found.types.isEmpty()) {
			return found.types.first()->size();
		}
		else {
			node = type->node();
			exprEvalError2(QString("Cannot find type BaseType for \"%1\" at %2:%3:%4")
						  .arg(type->toString())
						  .arg(_ast->fileName())
						  .arg(node ? node->start->line : 0)
						  .arg(node ? node->start->charPosition : 0),
						  node);
		}
		break;
	}

//	case rtConst:
//	case rtVolatile:

//	case rtTypedef:
	case rtFuncPointer:
	case rtFunction:
		return _eval->sizeofPointer();

	case rtVoid:
		return 1;

//	case rtVaList:
    default:
        node = type->node();
        exprEvalError2(QString("Cannot determine size of type \"%1\" at %2:%3:%4")
                      .arg(type->toString())
                      .arg(_ast->fileName())
                      .arg(node ? node->start->line : 0)
                      .arg(node ? node->start->charPosition : 0),
                      node);

    }

    return 0;
}


bool ASTExpressionEvaluator::appendPostfixExpressionSuffixes(
        const ASTNodeList* suffixList, ASTVariableExpression *varExpr)
{
    if (!varExpr)
        return false;

    // Append all postfix expression suffixes
    for (const ASTNodeList* list = suffixList; list; list = list->next)
    {
        const ASTNode* p = list->item;
        switch(p->type) {
        case nt_postfix_expression_arrow:
            varExpr->appendTransformation(ttDereference);
            // no break

        case nt_postfix_expression_dot:
            varExpr->appendTransformation(
                        antlrTokenToStr(
                            p->u.postfix_expression_suffix.identifier));
            break;

        case nt_postfix_expression_brackets: {
            // We expect array indices to be constant
            const ASTNode* e = p->u.postfix_expression_suffix.expression ?
                        p->u.postfix_expression_suffix.expression->item : 0;
            ASTExpression* index = exprOfNode(e, ASTNodeNodeHash());
            ExpressionResult result = index ?
                        index->result() : ExpressionResult();
            if (result.resultType == erConstant)
                varExpr->appendTransformation(result.value(esInt32));
            // Seems to be a runtime dependent expresssion
            else
                return false;
            break;
        }

            // Any other type cannot be resolved
        default:
            return false;
        }
    }

    return true;
}


ExpressionResultSize ASTExpressionEvaluator::realTypeToResultSize(RealType type)
{
    switch (type) {
    case rtInt8:   return esInt8;
    case rtUInt8:  return esUInt8;
    case rtInt16:  return esInt16;
    case rtUInt16: return esUInt16;
    case rtInt32:  return esInt32;
    case rtUInt32: return esUInt32;
    case rtInt64:  return esInt64;
    case rtUInt64: return esUInt64;
    case rtFloat:  return esFloat;
    case rtDouble: return esDouble;
    default:       return esUndefined;
    }
}


void ASTExpressionEvaluator::clearCache()
{
    _expressions.clear();
}
