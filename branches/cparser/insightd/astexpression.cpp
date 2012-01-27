
#include "astexpression.h"
#include "expressionevalexception.h"
#include "instance.h"
#include "structured.h"


const char* expressionTypeToString(ExpressionType type)
{
    switch (type) {
    case etVoid:                return "etVoid";
    case etUndefined:           return "etUndefined";
    case etRuntimeDependent:    return "etRuntimeDependent";
    case etLiteralConstant:     return "etLiteralConstant";
    case etEnumerator:          return "etEnumerator";
    case etVariable:            return "etVariable";
    case etLogicalOr:           return "etLogicalOr";
    case etLogicalAnd:          return "etLogicalAnd";
    case etInclusiveOr:         return "etInclusiveOr";
    case etExclusiveOr:         return "etExclusiveOr";
    case etAnd:                 return "etAnd";
    case etEquality:            return "etEquality";
    case etUnequality:          return "etUnequality";
    case etRelationalGE:        return "etRelationalGE";
    case etRelationalGT:        return "etRelationalGT";
    case etRelationalLE:        return "etRelationalLE";
    case etRelationalLT:        return "etRelationalLT";
    case etShiftLeft:           return "etShiftLeft";
    case etShiftRight:          return "etShiftRight";
    case etAdditivePlus:        return "etAdditivePlus";
    case etAdditiveMinus:       return "etAdditiveMinus";
    case etMultiplicativeMult:  return "etMultiplicativeMult";
    case etMultiplicativeDiv:   return "etMultiplicativeDiv";
    case etMultiplicativeMod:   return "etMultiplicativeMod";
    case etUnaryDec:            return "etUnaryDec";
    case etUnaryInc:            return "etUnaryInc";
    case etUnaryStar:           return "etUnaryStar";
    case etUnaryAmp:            return "etUnaryAmp";
    case etUnaryMinus:          return "etUnaryMinus";
    case etUnaryInv:            return "etUnaryInv";
    case etUnaryNot:            return "etUnaryNot";
    }
    return "UNKNOWN";
}


QString ASTVariableExpression::toString(bool /*compact*/) const
{
    if (!_baseType)
        return "(undefined var. expr.)";

    QString s = QString("(%1)").arg(_baseType->prettyName());

    for (int i = 0; i < _pel.size(); ++i) {
        switch (_pel[i].type) {
        case ptDot:
            s += "." + _pel[i].member;
            break;
        case ptArrow:
            s += "->" + _pel[i].member;
            break;
        case ptBrackets:
            s += QString("[%1]").arg(_pel[i].arrayIndex);
            break;
        }
    }

    return s;
}


bool ASTVariableExpression::equals(const ASTExpression *other) const
{
    if (!ASTExpression::equals(other))
        return false;
    const ASTVariableExpression* v =
            dynamic_cast<const ASTVariableExpression*>(other);
    // Compare base type
    if (!v || _baseType != v->_baseType || _pel.size() != v->_pel.size())
        return false;
    // Compare postfix expressions
    for (int i = 0; i < _pel.size(); ++i) {
        if (_pel[i].type != v->_pel[i].type ||
            _pel[i].arrayIndex != v->_pel[i].arrayIndex ||
            _pel[i].member != v->_pel[i].member)
            return false;
    }

    return true;
}


ExpressionResult ASTVariableExpression::result(const Instance *inst) const
{
    // Make sure we got a valid instance and type
    if (!inst || !_baseType)
        return ExpressionResult(erUndefined);
    // Make sure the type of the instance matches our type
    if (inst->type()->id() != _baseType->id()) {
        // Try to match all postfix expression suffixes
        bool match = false;
        const BaseType* bt = _baseType->dereferencedBaseType();
        QString prettyType = bt ? bt->prettyName() : QString("(undefined");
        for (int i = 0; bt && i < _pel.size(); ++i) {
            if (_pel[i].type == ptArrow || _pel[i].type == ptDot) {
                prettyType += "." + _pel[i].member;
                const Structured* s = dynamic_cast<const Structured*>(bt);
                if (!s)
                    exprEvalError(QString("Type %1 (0x%2) is not a structured "
                                          "type")
                                  .arg(prettyType)
                                  .arg((uint)bt->id(), 0, 16));
                if (!s->memberExists(_pel[i].member))
                    exprEvalError(QString("Type %1 has no member \"%2\"")
                                  .arg(prettyType)
                                  .arg(_pel[i].member));
                const StructuredMember* m = s->findMember(_pel[i].member);
                bt = m ? m->refTypeDeep(BaseType::trLexicalAndPointers) : 0;
            }
        }


        if (!match) {
            exprEvalError(QString("Type ID of instance (0x%1) is different "
                                  "from ours (0x%2)")
                          .arg((uint)inst->type()->id(), 0, 16)
                          .arg((uint)_baseType->id(), 0, 16));
            return ExpressionResult(erUndefined);
        }
    }

    if (_pel.isEmpty())
        return inst->toExpressionResult();

    // Apply the postfix expressions
    Instance tmp(*inst);
    int cnt = 0;
    for (int i = 0; i < _pel.size() && tmp.isValid(); ++i) {
        switch (_pel[i].type) {
        case ptArrow:
            // Dereference the instance exactly once
            tmp = tmp.dereference(BaseType::trLexicalAndPointers, 1, &cnt);
            // Make sure we succeeded in dereferencing the pointer
            if (cnt != 1 || !tmp.isValid())
                return ExpressionResult(erUndefined);
            // no break

        case ptDot:
            tmp = tmp.findMember(_pel[i].member, BaseType::trLexical);
            break;

        case ptBrackets:
            tmp = tmp.arrayElem(_pel[i].arrayIndex);
            break;
        }
    }

    return tmp.toExpressionResult();
}


ExpressionResult ASTBinaryExpression::result(const Instance *inst) const
{
    if (!_left || !_right)
        return ExpressionResult();

    ExpressionResult lr = _left->result(inst);
    ExpressionResult rr = _right->result(inst);
    ExpressionResult ret(lr.resultType | rr.resultType);
    ExpressionResultSize target = ret.size = binaryExprSize(lr, rr);
    // Is the expression decidable?
    if (lr.resultType != erConstant || rr.resultType != erConstant) {
        // Undecidable, so return the combined result type
        return ret;
    }

    bool isUnsigned = (ret.size & esUnsigned) ? true : false;

#define LOGICAL_OP(op) \
    do { \
        ret.size = esUInt8; \
        if (target & esInteger) { \
            if (isUnsigned) \
                ret.result.i32 = (lr.uvalue(target) op rr.uvalue(target)) ? \
                    1 : 0; \
            else \
                ret.result.i32 = (lr.value(target) op rr.value(target)) ? \
                    1 : 0; \
        } \
        else if (target & esFloat) \
            ret.result.i32 = (lr.fvalue() op rr.fvalue()) ? 1 : 0; \
        else if (target & esDouble) \
            ret.result.i32 = (lr.dvalue() op rr.dvalue()) ? 1 : 0; \
        else \
            exprEvalError(QString("Invalid target type: %1") \
                          .arg(target)); \
    } while (0)


#define BITWISE_LOGICAL_OP(op) \
    do { \
        if (target & esInteger) { \
            if (isUnsigned) \
                ret.result.ui64 = lr.uvalue(target) op rr.uvalue(target); \
            else \
                ret.result.i64 = lr.value(target) op rr.value(target); \
        } \
        else \
            exprEvalError(QString("Invalid operator \"%1\" for target type %2.") \
                          .arg(#op) \
                          .arg(target)); \
    } while (0)


#define SHIFT_OP(op) \
    do { \
        ret.size = lr.size; /* result is the type of the left hand */  \
        if (lr.size & esInteger) { \
            /* If right-hand operand is negative, the result is undefined. */  \
            if (lr.size & esUnsigned)  \
                ret.result.ui64 = lr.uvalue() op rr.value(); \
            else \
                ret.result.i64 = lr.value() op rr.value(); \
        } \
        else \
            exprEvalError(QString("Invalid operator \"%1\" for target type %2.") \
                          .arg(#op) \
                          .arg(lr.size)); \
    } while (0)

#define ADDITIVE_OP(op) \
    do { \
        if (target & esInteger) { \
            if (isUnsigned) \
                ret.result.ui64 = lr.uvalue(target) op rr.uvalue(target); \
            else \
                ret.result.i64 = lr.value(target) op rr.value(target); \
        } \
        else if (target & esFloat) \
            ret.result.f = lr.fvalue() op rr.fvalue(); \
        else if (target & esDouble) \
            ret.result.d = lr.dvalue() op rr.dvalue(); \
        else \
            exprEvalError(QString("Invalid target type: %1") \
                          .arg(target)); \
    } while (0)

#define MULTIPLICATIVE_OP(op) \
    do { \
        if (target & esInteger) { \
            if (lr.size & esUnsigned) { \
                if (rr.size & esUnsigned) \
                    ret.result.i64 = lr.uvalue(target) op rr.uvalue(target); \
                else \
                    ret.result.i64 = lr.uvalue(target) op rr.value(target); \
            } \
            else { \
                if (rr.size & esUnsigned) \
                    ret.result.i64 = lr.value(target) op rr.uvalue(target); \
                else \
                    ret.result.i64 = lr.value(target) op rr.value(target); \
            } \
        } \
        else if (target & esFloat) \
            ret.result.f = lr.fvalue() op rr.fvalue(); \
        else if (target & esDouble) \
            ret.result.d = lr.dvalue() op rr.dvalue(); \
        else \
            exprEvalError(QString("Invalid target type: %1") \
                          .arg(target)); \
    } while (0)

#define MULTIPLICATIVE_MOD(op) \
    do { \
        if (target & esInteger) { \
            if (lr.size & esUnsigned) { \
                if (rr.size & esUnsigned) \
                    ret.result.i64 = lr.uvalue(target) op rr.uvalue(target); \
                else \
                    ret.result.i64 = lr.uvalue(target) op rr.value(target); \
            } \
            else { \
                if (rr.size & esUnsigned) \
                    ret.result.i64 = lr.value(target) op rr.uvalue(target); \
                else \
                    ret.result.i64 = lr.value(target) op rr.value(target); \
            } \
        } \
        else \
            exprEvalError(QString("Invalid operator \"%1\" for target type %2.") \
                          .arg(#op) \
                          .arg(target)); \
    } while (0)


    switch (_type) {
    case etLogicalOr:
        LOGICAL_OP(||);
        break;

    case etLogicalAnd:
        LOGICAL_OP(&&);
        break;

    case etInclusiveOr:
        BITWISE_LOGICAL_OP(|);
        break;

    case etExclusiveOr:
        BITWISE_LOGICAL_OP(^);
        break;

    case etAnd:
        BITWISE_LOGICAL_OP(&);
        break;

    case etEquality:
        LOGICAL_OP(==);
        break;

    case etUnequality:
        LOGICAL_OP(!=);
        break;

    case etRelationalGE:
        LOGICAL_OP(>=);
        break;

    case etRelationalGT:
        LOGICAL_OP(>);
        break;

    case etRelationalLE:
        LOGICAL_OP(<=);
        break;

    case etRelationalLT:
        LOGICAL_OP(<);
        break;

    case etShiftLeft:
        SHIFT_OP(<<);
        break;

    case etShiftRight:
        SHIFT_OP(>>);
        break;

    case etAdditivePlus:
        ADDITIVE_OP(+);
        break;

    case etAdditiveMinus:
        ADDITIVE_OP(-);
        break;

    case etMultiplicativeMult:
        MULTIPLICATIVE_OP(*);
        break;

    case etMultiplicativeDiv:
        MULTIPLICATIVE_OP(/);
        break;

    case etMultiplicativeMod:
        MULTIPLICATIVE_MOD(%);
        break;

    default:
        ret.resultType = erUndefined;
    }

    return ret;
}



QString ASTBinaryExpression::operatorToString() const
{
    switch (_type) {
    case etLogicalOr:           return "||";
    case etLogicalAnd:          return "&&";
    case etInclusiveOr:         return "|";
    case etExclusiveOr:         return "^";
    case etAnd:                 return "&";
    case etEquality:            return "==";
    case etUnequality:          return "!=";
    case etRelationalGE:        return ">=";
    case etRelationalGT:        return ">";
    case etRelationalLE:        return "<=";
    case etRelationalLT:        return "<";
    case etShiftLeft:           return "<<";
    case etShiftRight:          return ">>";
    case etAdditivePlus:        return "+";
    case etAdditiveMinus:       return "-";
    case etMultiplicativeMult:  return "*";
    case etMultiplicativeDiv:   return "/";
    case etMultiplicativeMod:   return "%";
    default:                    return "??";
    }
}


QString ASTBinaryExpression::toString(bool compact) const
{
    if (compact) {
        ExpressionResult res = result();
        // Does the expression evaluate to a constant value?
        if (!(res.resultType & (erRuntime|erUndefined|erGlobalVar|erLocalVar)))
            return res.toString();
    }

    QString left = _left ? _left->toString(compact) : QString("(left unset)");
    QString right = _right ? _right->toString(compact) : QString("(right unset)");

    return QString("(%1 %2 %3)").arg(left).arg(operatorToString()).arg(right);
}


ExpressionResultSize ASTBinaryExpression::binaryExprSize(
        const ExpressionResult& r1, const ExpressionResult& r2)
{
    /*
    Many binary operators that expect operands of arithmetic or enumeration
    type cause conversions and yield result types in a similar way. The
    purpose is to yield a common type, which is also the type of the result.
    This pattern is called the usual arithmetic conversions, which are
    defined as follows:
    - If either operand is of type long double, the other shall be converted
      to long double.
    - Otherwise, if either operand is double, the other shall be converted
      to double.
    - Otherwise, if either operand is float, the other shall be converted to
      float.
    - Otherwise, the integral promotions (4.5) shall be performed on both
      operands.
    - Then, if either operand is unsigned long the other shall be converted
      to unsigned long.
    - Otherwise, if one operand is a long int and the other unsigned int,
      then if a long int can represent all the values of an unsigned int,
      the unsigned int shall be converted to a long int; otherwise both
      operands shall be converted to unsigned long int.
    - Otherwise, if either operand is long, the other shall be converted to
      long.
    - Otherwise, if either operand is unsigned, the other shall be converted
      to unsigned. [Note: otherwise, the only remaining case is that both
      operands are int ]

    Integral promotions:
    An rvalue of type char, signed char, unsigned char, short int, or
    unsigned short int can be converted to an rvalue of type int if int can
    represent all the values of the source type; otherwise, the source
    rvalue can be converted to an rvalue of type unsigned int.
    */

    int r1r2_size = r1.size | r2.size;
    // If either is double, the result is double
    if (r1r2_size & esDouble)
        return esDouble;
    // Otherwise, if either is float, the result is float
    else if (r1r2_size & esFloat)
        return esFloat;
    // Otherwise
    else {
        // Integral promition
        ExpressionResultSize r1_size = (r1.size & (es8Bit|es16Bit)) ?
                    esInt32 : r1.size;
        ExpressionResultSize r2_size = (r2.size & (es8Bit|es16Bit)) ?
                    esInt32 : r2.size;
        r1r2_size = r1_size | r2_size;

        // If either is unsigned long, the result is unsigned long
        if (r1_size == esUInt64 || r2_size == esUInt64)
            return esUInt64;
        // Otherwise, if one is long and the other unsigned, result is long
        // Otherwise, if either is long, the result is long.
        else if (r1r2_size & esInt64)
            return esInt64;
        // Otherwise, if either is unsigned, the result is unsigned
        else if (r1r2_size & esUnsigned)
            return esUInt32;
    }
    return esInt32;
}


ExpressionResult ASTUnaryExpression::result(const Instance *inst) const
{
    if (!_child)
        return ExpressionResult();

    ExpressionResult res = _child->result(inst);
    // Is the expression decidable?
    if (res.resultType & (erUndefined|erRuntime)) {
        /// @todo constants cannot be incremented, that makes no sense at all!
        // Undecidable, so return the correct result type
        res.resultType |= erUndefined;
        return res;
    }

#define UNARY_PREFIX(op) \
    do { \
        if (res.size & esInteger) { \
            if (res.size & es64Bit) \
                op res.result.ui64; \
            else if (res.size & es32Bit) \
                op res.result.ui32; \
            else if (res.size & es16Bit) \
                op res.result.ui16; \
            else \
                op res.result.ui8; \
        } \
        else \
            exprEvalError(QString("Invalid operator \"%1\" for target type %2.") \
                          .arg(#op) \
                          .arg(res.size)); \
    } while (0)


    switch (_type) {
    case etUnaryInc:
        UNARY_PREFIX(++);
        break;

    case etUnaryDec:
        UNARY_PREFIX(--);
        break;

        /// @todo Fixme
//        case etUnaryStar:
//        case etUnaryAmp:

    case etUnaryMinus:
        if (res.size & esInteger) {
            if (res.size & esUnsigned)
                res.result.ui64 = -res.result.ui64;
            else
                res.result.i64 = -res.result.i64;
        }
        else if (res.size & esDouble)
            res.result.d = -res.result.d;
        else if (res.size & esFloat)
            res.result.f = -res.result.f;
        else
            exprEvalError(QString("Invalid target type: %1").arg(res.size));
        break;

    case etUnaryInv:
        if (res.size & esInteger) {
            // Extend smaller values to 32 bit
            if (res.size & (es8Bit|es16Bit)) {
                res.result.i32 = ~res.value(esInt32);
                res.size = esInt32;
            }
            else {
                // I don't understand why, but GCC changes any int64 to unsigned
                // if the value was negative before, so just play the game...
                if (res.size == esInt64 && res.result.i64 < 0)
                    res.size = esUInt64;
                res.result.ui64 = ~res.result.ui64;
            }
        }
        else
            exprEvalError(QString("Invalid operator \"%1\" for target type %2")
                          .arg("~")
                          .arg(res.size));
        break;

    case etUnaryNot:
        if (res.size & esInteger)
            res.result.i32 = (!res.uvalue()) ? 1 : 0;
        else if (res.size & esDouble)
            res.result.i32 = (!res.dvalue()) ? 1 : 0;
        else if (res.size & esFloat)
            res.result.i32 = (!res.fvalue()) ? 1 : 0;
        else
            exprEvalError(QString("Invalid target type: %1").arg(res.size));
        res.size = esUInt8;
        break;

    default:
        exprEvalError(QString("Unhandled unary expression: %1")
                      .arg(expressionTypeToString(_type)));
    }

    return res;
}


QString ASTUnaryExpression::toString(bool compact) const
{
    if (compact) {
        ExpressionResult res = result();
        // Does the expression evaluate to a constant value?
        if (!(res.resultType & (erRuntime|erUndefined|erGlobalVar|erLocalVar)))
            return res.toString();
    }

    QString child = _child ? _child->toString(compact) : QString("(child unset)");

    return QString("%1%2").arg(operatorToString()).arg(child);
}


QString ASTUnaryExpression::operatorToString() const
{
    switch (_type) {
    case etUnaryInc:   return "++";
    case etUnaryDec:   return "--";
    case etUnaryMinus: return "-";
    case etUnaryInv:   return "~";
    case etUnaryNot:   return "!";
    case etUnaryStar:  return "*";
    case etUnaryAmp:   return "&";
    default:           return "??";
    }
}

