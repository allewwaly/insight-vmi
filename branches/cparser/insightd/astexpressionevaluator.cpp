#include "astexpressionevaluator.h"
#include <abstractsyntaxtree.h>
#include "astexpression.h"
#include "expressionevalexception.h"
#include <astnode.h>
#include <asttypeevaluator.h>
#include <astsourceprinter.h>
#include "symfactory.h"
#include "basetype.h"
#include "array.h"

#define checkNodeType(node, expected_type) \
    if ((node)->type != (expected_type)) { \
            exprEvalError( \
                QString("Expected node type \"%1\", given type is \"%2\" at %3:%4") \
                    .arg(ast_node_type_to_str2(expected_type)) \
                    .arg(ast_node_type_to_str(node)) \
                    .arg(_ast->fileName()) \
                    .arg(node->start->line)); \
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


ASTExpression* ASTExpressionEvaluator::exprOfNodeList(const ASTNodeList *list)
{
    ASTExpression* expr = 0;
    while (list) {
        if (expr)
            expr->addAlternative(exprOfNode(list->item));
        else
            expr = exprOfNode(list->item);
        list = list->next;
    }
    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfNode(const ASTNode *node)
{
    if (!node)
        return 0;

    // Return cached value, if possible
    if (_expressions.contains(node))
        return _expressions[node];

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
        expr = exprOfBinaryExpr(node);
        break;

    case nt_assignment_expression:
        expr = exprOfAssignmentExpr(node);
        break;

    case nt_builtin_function_alignof:
        expr = exprOfBuiltinFuncAlignOf(node);
        break;

    case nt_builtin_function_choose_expr:
        expr = exprOfBuiltinFuncChooseExpr(node);
        break;

    case nt_builtin_function_constant_p:
        expr = exprOfBuiltinFuncConstant(node);
        break;

    case nt_builtin_function_expect:
        expr = exprOfBuiltinFuncExpect(node);
        break;

    case nt_builtin_function_extract_return_addr:
        expr = createExprNode<ASTRuntimeExpression>();
        break;

    case nt_builtin_function_object_size:
        expr = exprOfBuiltinFuncObjectSize(node);
        break;

    case nt_builtin_function_offsetof:
        expr = exprOfBuiltinFuncOffsetOf(node);
        break;

    case nt_builtin_function_prefetch:
        expr = createExprNode<ASTVoidExpression>();
        break;

    case nt_builtin_function_return_address:
        expr = createExprNode<ASTRuntimeExpression>();
        break;

    case nt_builtin_function_sizeof:
        expr = exprOfBuiltinFuncSizeof(node);
        break;

    case nt_builtin_function_types_compatible_p:
        expr = exprOfBuiltinFuncTypesCompatible(node);
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
            return exprOfNode(node->u.cast_expression.cast_expression);
        }
        else
            return exprOfNode(node->u.cast_expression.unary_expression);
        break;

    case nt_conditional_expression:
        expr = exprOfConditionalExpr(node);
        break;

    case nt_constant_char:
    case nt_constant_int:
        expr = exprOfConstant(node);
        break;

    case nt_lvalue:
        // Could be an lvalue cast
        if (node->u.lvalue.lvalue)
            expr = exprOfNode(node->u.lvalue.lvalue);
        else
            expr = exprOfNode(node->u.lvalue.unary_expression);
        break;

    case nt_postfix_expression:
        expr = exprOfPostfixExpr(node);
        break;

    case nt_primary_expression:
        expr = exprOfPrimaryExpr(node);
        break;

    case nt_unary_expression:
    case nt_unary_expression_builtin:
    case nt_unary_expression_dec:
    case nt_unary_expression_inc:
    case nt_unary_expression_op:
        expr = exprOfUnaryExpr(node);
        break;

    default:
        exprEvalError(QString("Unexpexted node type: %1 at %2:%3:%4")
                      .arg(ast_node_type_to_str(node))
                      .arg(_ast ? _ast->fileName() : QString())
                      .arg(node->start->line)
                      .arg(node->start->charPosition));
    }

    if (!expr) {
        exprEvalError(QString("Failed to evaluate node %1 at %2:%3:%4")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line)
                .arg(node->start->charPosition));
    }

    _expressions[node] = expr;
    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfAssignmentExpr(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_assignment_expression);

    if (node->u.assignment_expression.assignment_expression)
        return exprOfNode(node->u.assignment_expression.assignment_expression);
    else if (node->u.assignment_expression.lvalue)
        return exprOfNode(node->u.assignment_expression.lvalue);
    else
        return exprOfNode(node->u.assignment_expression.conditional_expression);
}


ASTExpression* ASTExpressionEvaluator::exprOfConditionalExpr(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_conditional_expression);

    if (node->u.conditional_expression.conditional_expression) {
        // Evaluate condition
        ASTExpression* ret = 0, *tmp = 0;

        for (ASTExpression* expr = exprOfNode(
                    node->u.conditional_expression.logical_or_expression);
             expr;
             expr = expr->alternative())
        {
            // For constant conditions we can choose the correct path
            if (expr->resultType() == erConstant) {
                if (expr->result().result.i64)
                    tmp = exprOfNodeList(
                                node->u.conditional_expression.expression);
                else
                    tmp = exprOfNode(node->u.conditional_expression.conditional_expression);
                ret = setExprOrAddAlternative(ret, tmp);
            }
            // Otherwise add both possibilities as alternatives
            else {
                tmp = exprOfNodeList(node->u.conditional_expression.expression);
                setExprOrAddAlternative(ret, tmp);
                tmp = exprOfNode(
                            node->u.conditional_expression.conditional_expression);
                setExprOrAddAlternative(ret, tmp);
            }
        }

        return ret;
    }
    else
        return exprOfNode(node->u.conditional_expression.logical_or_expression);
}


ASTExpression* ASTExpressionEvaluator::exprOfBinaryExpr(const ASTNode *node)
{
    if (!node)
        return 0;

    if (!node->u.binary_expression.right)
        return exprOfNode(node->u.binary_expression.left);

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
        else
            exprType = etMultiplicativeDiv;
        break;
    default:
        exprEvalError(QString("Type \"%1\" represents no binary expression at %2:%3:%4")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line)
                .arg(node->start->charPosition));
    }

    ASTBinaryExpression *expr = createExprNode<ASTBinaryExpression>(exprType);
    expr->setLeft(exprOfNode(node->u.binary_expression.left));
    expr->setRight(exprOfNode(node->u.binary_expression.right));
    return expr;
}


ASTExpression* ASTExpressionEvaluator::exprOfUnaryExpr(const ASTNode *node)
{
    if (!node)
        return 0;

    ASTUnaryExpression* ue = 0;

    switch (node->type) {
    case nt_unary_expression:
        return exprOfNode(node->u.unary_expression.postfix_expression);

    case nt_unary_expression_builtin:
        return exprOfNode(node->u.unary_expression.builtin_function);

    case nt_unary_expression_dec:
        ue = createExprNode<ASTUnaryExpression>(etUnaryDec);
        ue->setChild(exprOfNode(node->u.unary_expression.unary_expression));
        return ue;

    case nt_unary_expression_inc:
        ue = createExprNode<ASTUnaryExpression>(etUnaryInc);
        ue->setChild(exprOfNode(node->u.unary_expression.unary_expression));
        return ue;

    case nt_unary_expression_op: {
        QString op = antlrTokenToStr(node->u.unary_expression.unary_operator);
        if (op == "&")
            ue = createExprNode<ASTUnaryExpression>(etUnaryAmp);
        else if (op == "&&") {
            // Double ampersand operator
            ASTUnaryExpression* ue2 =
                    createExprNode<ASTUnaryExpression>(etUnaryAmp);
            ue2->setChild(exprOfNode(node->u.unary_expression.cast_expression));
            ue = createExprNode<ASTUnaryExpression>(etUnaryAmp);
            ue->setChild(ue2);
            return ue;
        }
        else if (op == "*")
            ue = createExprNode<ASTUnaryExpression>(etUnaryStar);
        else if (op == "+")
            return exprOfNode(node->u.unary_expression.cast_expression);
        else if (op == "-")
            ue = createExprNode<ASTUnaryExpression>(etUnaryMinus);
        else if (op == "~")
            ue = createExprNode<ASTUnaryExpression>(etUnaryInv);
        else if (op == "!")
            ue = createExprNode<ASTUnaryExpression>(etUnaryNot);
        else
            exprEvalError(QString("Unhandled unary operator: \"%1\" at %2:%3:%4")
                    .arg(op)
                    .arg(_ast->fileName())
                    .arg(node->start->line)
                    .arg(node->start->charPosition));

        ue->setChild(exprOfNode(node->u.unary_expression.cast_expression));
        break;
    }

    default:
        exprEvalError(QString("Type \"%1\" represents no unary expression at %2:%3:%4")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line)
                .arg(node->start->charPosition));
    }

    return ue;
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
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncAlignOf(const ASTNode *node)
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
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncChooseExpr(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_choose_expr);

    ASTExpression *ret = 0, *ae1 = 0, *ae2;

    for (ASTExpression *expr = exprOfNode(
                node->u.builtin_function_choose_expr.constant_expression);
         expr;
         expr = expr->alternative())
    {
        ExpressionResult res = expr->result();
        // Is the result valid?
        if (res.resultType == erConstant) {
            const ASTNode *n = res.result.i64 ?
                        node->u.builtin_function_choose_expr.assignment_expression1 :
                        node->u.builtin_function_choose_expr.assignment_expression2;
            ae1 = exprOfNode(n);
            ret = setExprOrAddAlternative(ret, ae1);
        }
        // If not, add both alternatives
        else {
            ae1 = exprOfNode(
                        node->u.builtin_function_choose_expr.assignment_expression1);
            ae2 = exprOfNode(
                        node->u.builtin_function_choose_expr.assignment_expression2);
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
        const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_object_size);

    ASTExpression *expr = exprOfNode(node->u.builtin_function_object_size.constant);
    ExpressionResult res = expr->result();
    assert(res.resultType == erConstant);

    ASTConstantExpression* ret =
            createExprNode<ASTConstantExpression>(
                _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                0);
    // If the maximum remaining bytes are requested, return (size_t) -1 instead
    // of 0.
    if (res.result.ui64 & 2)
        ret->setValue(res.size, -1);
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
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncConstant(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_constant_p);

    ASTExpression *expr = exprOfNode(
                node->u.builtin_function_constant_p.unary_expression);
    // We can only approximate GCC's constant value decision here
    return expr->resultType() == etConstant ?
                createExprNode<ASTConstantExpression>(esInt32, 1) :
                createExprNode<ASTConstantExpression>(esInt32, 0);

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
ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncExpect(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_constant_p);

    return exprOfNode(node->u.builtin_function_expect.assignment_expression);
}


ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncOffsetOf(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_builtin_function_offsetof);

    const ASTNode *pfe = node->u.builtin_function_offsetof.postfix_expression;
    const ASTNode* pre = pfe->u.postfix_expression.primary_expression;
    ASTType* type = _eval->typeofNode(node->u.builtin_function_offsetof.type_name);
    if (!type || !(type->type() & StructOrUnion)) {
        ASTSourcePrinter printer(_ast);
        exprEvalError(QString("Cannot find struct/union definition for \"%1\" "
                              "at %2:%3:%4")
                      .arg(printer.toString(pfe).trimmed())
                      .arg(_ast->fileName())
                      .arg(node->start->line)
                      .arg(node->start->charPosition));
    }

    quint64 offset = 0;
    QString name = antlrTokenToStr(pre->u.primary_expression.identifier);
    const ASTNode *arrayIndexExpr = 0;
    const ASTNodeList *pfesl =
            pfe->u.postfix_expression.postfix_expression_suffix_list;
    BaseType* bt = 0;
    BaseTypeList bt_list = _factory->findBaseTypesForAstType(type, _eval).second;
    for (int i = 0; !bt && i < bt_list.size(); ++i)
        if (bt_list[i]->type() & StructOrUnion)
            bt = bt_list[i];

    Structured* s = 0;
    StructuredMember* m = 0;

    do {
        // Offset of array operator to member
        if (arrayIndexExpr) {
            // We should have found a member already
            assert(m != 0);
            assert(bt != 0);
            // The type of bt should be rtArray
            Array* a = dynamic_cast<Array*>(bt);
            if (!a)
                exprEvalError(QString("Type \"%1\" is not an array at %2:%3:%4")
                              .arg(bt ? bt->prettyName() : QString("NULL"))
                              .arg(_ast->fileName())
                              .arg(pfesl ?
                                       pfesl->item->start->line :
                                       pfe->start->line)
                              .arg(pfesl ?
                                       pfesl->item->start->charPosition :
                                       pfe->start->charPosition));
            // The new BaseType is the array's referencing type
            bt = a->refTypeDeep(BaseType::trLexical);

            ASTExpression* expr = exprOfNode(arrayIndexExpr);
            ExpressionResult index = expr->result();
            // We should find at least one constant expression
            while (expr->hasAlternative() && index.resultType != etConstant) {
                expr = expr->alternative();
                index = expr->result();
            }
            // Make sure we can evaluate the expression
            if (index.resultType != etConstant) {
                ASTSourcePrinter printer(_ast);
                exprEvalError(QString("Expression in brackets is not constant "
                                      "in \"%1\" at %2:%3:%4")
                              .arg(printer.toString(pfe).trimmed())
                              .arg(_ast->fileName())
                              .arg(arrayIndexExpr->start->line)
                              .arg(arrayIndexExpr->start->charPosition));
            }
            // Add array offset to total offset
            offset += index.result.ui64 * bt->size();
            // Dereference
        }
        // Offset of member by name
        else {
            s = dynamic_cast<Structured*>(bt);
            if (!s)
                exprEvalError(QString("Cannot find type struct/union BaseType "
                                      "for \"%1\" at %2:%3:%4")
                              .arg(type->toString())
                              .arg(_ast->fileName())
                              .arg(pfesl ?
                                       pfesl->item->start->line :
                                       pfe->start->line)
                              .arg(pfesl ?
                                       pfesl->item->start->charPosition :
                                       pfe->start->charPosition));

            m = s->findMember(name);
            if (!m)
                exprEvalError(QString("Cannot find member \"%1\" in %2 at "
                                      "%3:%4:%5")
                              .arg(name)
                              .arg(s->prettyName())
                              .arg(_ast->fileName())
                              .arg(pfesl ?
                                       pfesl->item->start->line :
                                       pfe->start->line)
                              .arg(pfesl ?
                                       pfesl->item->start->charPosition :
                                       pfe->start->charPosition));
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
                exprEvalError(QString("Unexpected postfix expression suffix "
                                      "type: %1 at %2:%3:%4")
                              .arg(ast_node_type_to_str(pfesl->item))
                              .arg(_ast->fileName())
                              .arg(node->start->line)
                              .arg(node->start->charPosition));

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


ASTExpression* ASTExpressionEvaluator::exprOfBuiltinFuncSizeof(const ASTNode *node)
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
                    _eval->sizeofLong());
    // Return alternatives for all sizes
    BaseTypeList list = _factory->findBaseTypesForAstType(type, _eval).second;
    QSet<quint32> sizes;
    ASTExpression *ret = 0;
    for (int i = 0; i < list.size(); ++i) {
        quint32 size = list[i]->size();
        // Add each size greater zero only once
        if (size > 0 && !sizes.contains(size)) {
            ASTExpression *expr =
                    createExprNode<ASTConstantExpression>(
                        _eval->sizeofLong() == 4 ? esUInt32 : esUInt64,
                        size);
            ret = setExprOrAddAlternative(ret, expr);
            sizes.insert(size);
        }
    }

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
        const ASTNode *node)
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
    return t1->equalTo(t2) ?
                createExprNode<ASTConstantExpression>(esInt32, 1) :
                createExprNode<ASTConstantExpression>(esInt32, 0);
}


ASTExpression* ASTExpressionEvaluator::exprOfConstant(const ASTNode *node)
{
    if (!node)
        return 0;

    bool ok = false;
    quint64 value;
    if (node->type == nt_constant_int) {
        QString s = antlrTokenToStr(node->u.constant.literal);
        QString suffix;
        // Remove any size or signedness specifiers from the string
        while (s.endsWith('l', Qt::CaseInsensitive) ||
               s.endsWith('u', Qt::CaseInsensitive))
        {
            suffix.prepend(s.right(1));
            s = s.left(s.size() - 1);
        }
        // Use the C convention to choose the correct base
        value = s.toULongLong(&ok, 0);
        if (!ok)
            exprEvalError(QString("Failed to parse constant value \"%1\" "
                                  "at %2:%3:%4")
                          .arg(s)
                          .arg(_ast->fileName())
                          .arg(node->start->line)
                          .arg(node->start->charPosition));
        ExpressionResultSize size = esUndefined;
        // Without suffix find the smallest type fitting the value
        if (suffix.isEmpty()) {
            if (value < (1ULL << 31))
                size = esInt32;
            else if (value < (1ULL << 32))
                size = esUInt32;
            else if (value < (1ULL << 63))
                size = esInt64;
            else
                size = esUInt64;
        }
        // Check explicit size and constant suffixes
        else {
            suffix = suffix.toLower();
            // Extend the size to 64 bit, depending on the number of l's and
            // the architecture of the guest
            if (suffix.endsWith("ll") ||
                (suffix.endsWith('l') && _eval->sizeofLong() > 4))
                size = esInt64;
            else
                size = esInt32;
            // Honor the unsigned flag. If not present, the constant is signed.
            if (suffix.startsWith('u'))
                size = (ExpressionResultSize) (size|esUnsigned);
        }
        return createExprNode<ASTConstantExpression>(size, value);
    }
    else {
        exprEvalError(QString("Unexpected constant value type: %1 at %2:%3:%4")
                      .arg(ast_node_type_to_str(node))
                      .arg(_ast->fileName())
                      .arg(node->start->line)
                      .arg(node->start->charPosition));
    }
}


ASTExpression* ASTExpressionEvaluator::exprOfPostfixExpr(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_postfix_expression);

    if (!node->u.postfix_expression.postfix_expression_suffix_list)
        return exprOfNode(node->u.postfix_expression.primary_expression);
    else
        // Keep it simple for now
        exprEvalError("We do not hande postfix expressions properly!");
}


ASTExpression* ASTExpressionEvaluator::exprOfPrimaryExpr(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_primary_expression);

    if (node->u.primary_expression.expression)
        return exprOfNode(node->u.primary_expression.expression->item);
    else if (node->u.primary_expression.identifier &&
             !node->u.primary_expression.hasDot)
    {
        const ASTSymbol* sym = _eval->findSymbolOfPrimaryExpression(node);
        return createExprNode<ASTVariableExpression>(sym);
    }
    else if (node->u.primary_expression.constant)
        return exprOfNode(node->u.primary_expression.constant);
    else
        exprEvalError(QString("Unexpected primary expression at %2:%3:%4")
                      .arg(_ast->fileName())
                      .arg(node ? node->start->line : 0)
                      .arg(node ? node->start->charPosition : 0));
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
		return _eval->sizeofLong();

	/// @todo
//	case rtArray:
//		break;

	case rtEnum:
		return 4;

	case rtStruct:
	case rtUnion: {
		// Find type in the factory
		AstBaseTypeList baseTypes =
				_factory->findBaseTypesForAstType(type, _eval);
		if (!baseTypes.second.isEmpty()) {
			return baseTypes.second.first()->size();
		}
		else {
			node = type->node();
			exprEvalError(QString("Cannot find type BaseType for \"%1\" at %2:%3:%4")
						  .arg(type->toString())
						  .arg(_ast->fileName())
						  .arg(node ? node->start->line : 0)
						  .arg(node ? node->start->charPosition : 0));
		}
		break;
	}

//	case rtConst:
//	case rtVolatile:

//	case rtTypedef:
	case rtFuncPointer:
	case rtFunction:
		return _eval->sizeofLong();

	case rtVoid:
		return 1;

//	case rtVaList:
    default:
        node = type->node();
        exprEvalError(QString("Cannot determine size of type \"%1\" at %2:%3:%4")
                      .arg(type->toString())
                      .arg(_ast->fileName())
                      .arg(node ? node->start->line : 0)
                      .arg(node ? node->start->charPosition : 0));

    }

    return 0;
}
