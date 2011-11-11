
#include "astexpression.h"
#include "expressionevalexception.h"


const char* expressionTypeToString(ExpressionType type)
{
    switch (type) {
    case etVoid:                return "etVoid";
    case etRuntimeDependent:    return "etRuntimeDependent";
    case etConstant:            return "etConstant";
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


qint64 ExpressionResult::value(ExpressionResultSize target) const
{
    qint64 ret;

    // Is either the target or this value unsigned?
    if (target && ((target | size) & esUnsigned)) {
        // If the target is unsigned, return the unsigned  value instead.
        // If this value is unsigned, do not extend the sign!
        ret = uvalue((ExpressionResultSize)(target | esUnsigned));
    }
    // No target specified, or both target and this value are signed, so
    // extend this value's sign to 64 bit
    else {
        ret = (size & es64Bit) ?
                    result.i64 :
                    (size & es32Bit) ?
                        (qint64) result.i32 :
                        (size & es16Bit) ?
                            (qint64) result.i16 :
                            (qint64) result.i8;
    }
//        debugmsg(QString("Returning 0x%1").arg(ret, 16, 16, QChar('0')));
    return ret;
}


quint64 ExpressionResult::uvalue(ExpressionResultSize target) const
{
    quint64 ret;

    // Is either the target or this value unsigned?
    if (target && !(target & size & esUnsigned)) {
        // If the target is signed, return the signed value instead
        if (!(target & esUnsigned))
            ret = value(target);
        // If this is a signed value and the target is unsigned, extend the
        // sign up to target size before converting to unsigned
        else {
            switch (target & (es64Bit|es32Bit|es16Bit|es8Bit)) {
            case es64Bit:
                ret = (size & es64Bit) ?
                            result.i64 :
                            (size & es32Bit) ?
                                (quint64) result.i32 :
                                (size & es16Bit) ?
                                    (quint64) result.i16 :
                                    (quint64) result.i8;
                break;

            case es32Bit:
                ret = (size & (es64Bit|es32Bit)) ?
                            result.ui32 :
                            (size & es16Bit) ?
                                (quint32) result.i16 :
                                (quint32) result.i8;
                break;

            case es16Bit:
                ret = (size & (es64Bit|es32Bit|es16Bit)) ?
                            result.ui16 :
                            (quint16) result.i8;
                break;

            case es8Bit:
                ret = result.ui8;
                break;

            default:
                exprEvalError(QString("Invalid target size: %1").arg(target));
                break;
            }
        }
    }
    else {
        ret = (size & es64Bit) ?
                result.ui64 :
                (size & es32Bit) ?
                    (quint64) result.ui32 :
                    (size & es16Bit) ?
                        (quint64) result.ui16 :
                        (quint64) result.ui8;
    }
//        debugmsg(QString("Returning 0x%1").arg(ret, 16, 16, QChar('0')));
    return ret;
}


float ExpressionResult::fvalue() const
{
    // Convert any value to float
    if (size & esDouble)
        return result.d;
    else if (size & esFloat)
        return result.f;
    else if (size & esUnsigned)
        return uvalue();
    else
        return value();
}


double ExpressionResult::dvalue() const
{
    // Convert any value to double
    if (size & esDouble)
        return result.d;
    else if (size & esFloat)
        return result.f;
    else if (size & esUnsigned)
        return uvalue();
    else
        return value();
}


ExpressionResult ASTBinaryExpression::result() const
{
    if (!_left || !_right)
        return ExpressionResult();

    ExpressionResult lr = _left->result();
    ExpressionResult rr = _right->result();
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
        ret.size = esInt32; \
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


ExpressionResult ASTUnaryExpression::result() const
{
    if (!_child)
        return ExpressionResult();

    ExpressionResult res = _child->result();
    // Is the expression decidable?
    if (res.resultType != erConstant) {
        /// @todo constants cannot be incremented, that makes no sense at all!
        // Undecidable, so return the correct result type
        res.resultType |= erInvalid;
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
        if (res.size & esInteger)
            res.result.ui64 = ~res.result.ui64;
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
        res.size = esInt32;
        break;

    default:
        exprEvalError(QString("Unhandled unary expression: %1")
                      .arg(expressionTypeToString(_type)));
    }

    return res;
}
