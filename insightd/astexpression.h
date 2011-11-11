#ifndef ASTEXPRESSION_H
#define ASTEXPRESSION_H

#include <QtGlobal>
#include <astsymbol.h>

/// Different types of expressions
enum ExpressionType {
    etVoid,
    etRuntimeDependent,
    etConstant,
    etVariable,
    etLogicalOr,
    etLogicalAnd,
    etInclusiveOr,
    etExclusiveOr,
    etAnd,
    etEquality,
    etUnequality,
    etRelationalGE,
    etRelationalGT,
    etRelationalLE,
    etRelationalLT,
    etShiftLeft,
    etShiftRight,
    etAdditivePlus,
    etAdditiveMinus,
    etMultiplicativeMult,
    etMultiplicativeDiv,
    etMultiplicativeMod,
    etUnaryDec,
    etUnaryInc,
    etUnaryStar,
    etUnaryAmp,
    etUnaryMinus,
    etUnaryInv,
    etUnaryNot
};

/**
 The type of an epxression result, which may be a bit-wise combination of the
 following enumeration values.
 */
enum ExpressionResultType {
    erUndefined = 0,         ///< Result is undefined
    erConstant  = (1 << 0),  ///< Expression is compile-time constant
    erGlobalVar = (1 << 1),  ///< Expression involves global variable
    erLocalVar  = (1 << 2),  ///< Expression involves local variable
    erParameter = (1 << 3),  ///< Expression involves function parameters
    erRuntime   = (1 << 4),  ///< Expression involves run-time dependencies
    erInvalid   = (1 << 5),  ///< Expression result cannot be determined
    erVoid      = (1 << 6)   ///< Expression result is void
};

enum ExpressionResultSize {
    esUndefined = 0,
    es8Bit      = (1 << 1),
    es16Bit     = (1 << 2),
    es32Bit     = (1 << 3),
    es64Bit     = (1 << 4),
    esUnsigned  = (1 << 5),
    esFloat     = (1 << 6),
    esDouble    = (1 << 7),
    esInt8      = es8Bit,
    esUInt8     = es8Bit|esUnsigned,
    esInt16     = es16Bit,
    esUInt16    = es16Bit|esUnsigned,
    esInt32     = es32Bit,
    esUInt32    = es32Bit|esUnsigned,
    esInt64     = es64Bit,
    esUInt64    = es64Bit|esUnsigned
};


/// The result of an expression
struct ExpressionResult
{
    /// Constructor
    ExpressionResult() : resultType(erUndefined), size(esInt32) { this->result.i64 = 0; }
    ExpressionResult(int resultType)
        : resultType(resultType), size(esInt32) { this->result.i64 = 0; }
    ExpressionResult(int resultType, ExpressionResultSize size, quint64 result)
        : resultType(resultType), size(size) { this->result.ui64 = result; }

    /// ORed combination of ExpressionResultType values
    int resultType;

    /// size of result of expression. \sa ExpressionResultSize
    ExpressionResultSize size;

    /// Expression result, if valid
    union Result {
        quint64 ui64;
        qint64 i64;
        quint32 ui32;
        qint32 i32;
        quint16 ui16;
        qint16 i16;
        quint8 ui8;
        qint8 i8;
        float f;
        double d;
    } result;

    inline qint64 value(ExpressionResultSize target = esUndefined) const
    {
        // If no target is specified or our own size is 64 bit or own type is
        // not unsigned, just sign-extend the signed value to 64 bit
        if (!target || (size & es64Bit) || !(size & esUnsigned)) {
            return (size & es64Bit) ?
                        result.i64 :
                        (size & es32Bit) ?
                            (qint64) result.i32 :
                            (size & es16Bit) ?
                                (qint64) result.i16 :
                                (qint64) result.i8;
        }
        // Target is specified, our type is unsigned and 32 bit or less
        else {
            // If our unsigned type fits into signed target type, then use it
            if ((target & es64Bit) ||
                ((target & es32Bit) && (size & (es16Bit|es8Bit))) ||
                ((target & es16Bit) && (size & es8Bit)))
                return uvalue();
            // Otherwise return sign-extended unsigned value, should actually
            // never happen
            else
                return value();
        }
    }

    inline quint64 uvalue(ExpressionResultSize target = esUndefined) const
    {
        // If we accidently got called for an unsigned target, return this value
        if (target && (!(target & esUnsigned) || !(size & esUnsigned)))
            return value((ExpressionResultSize) (target & ~esUnsigned));
        return (size & es64Bit) ?
                    result.ui64 :
                    (size & es32Bit) ?
                        (quint64) result.ui32 :
                        (size & es16Bit) ?
                            (quint64) result.ui16 :
                            (quint64) result.ui8;
    }

    inline float fvalue() const
    {
        return result.f;
    }

    inline double dvalue() const
    {
        return result.d;
    }
};

/**
 * Abstract base class for a syntax tree expression.
 */
class ASTExpression
{
public:
    ASTExpression() : _alternative(0) {}

    virtual ExpressionType type() const = 0;
    virtual int resultType() const = 0;
    virtual ExpressionResult result() = 0;

    inline bool hasAlternative() const
    {
        return _alternative != 0;
    }

    inline ASTExpression* alternative() const
    {
        return _alternative;
    }

    inline void addAlternative(ASTExpression* alt)
    {
        // Avoid dublicates
        if (_alternative == alt)
            return;
        if (_alternative)
            _alternative->addAlternative(alt);
        else
            _alternative = alt;
    }

protected:
    ASTExpression* _alternative;
};

/**
 * This expression returns nothing and cannot be evaluated.
 */
class ASTVoidExpression: public ASTExpression
{
public:
    inline virtual ExpressionType type() const
    {
        return etVoid;
    }

    inline virtual int resultType() const
    {
        return erInvalid|erRuntime;
    }

    inline virtual ExpressionResult result()
    {
        return ExpressionResult(resultType(), esInt32, 0);
    }
};

/**
 * This expression involves run-time depencencies and cannot be evaluated.
 */
class ASTRuntimeExpression: public ASTExpression
{
public:
    inline virtual ExpressionType type() const
    {
        return etRuntimeDependent;
    }

    inline virtual int resultType() const
    {
        return erRuntime;
    }

    inline virtual ExpressionResult result()
    {
        return ExpressionResult(resultType(), esInt32, 0);
    }
};

/**
 * This expression represents a compile-time constant value.
 */
class ASTConstantExpression: public ASTExpression
{
public:
    ASTConstantExpression() {}
    ASTConstantExpression(ExpressionResultSize size, quint64 value)
        : _value(erConstant, size, value) {}

    inline virtual ExpressionType type() const
    {
        return etConstant;
    }

    inline virtual int resultType() const
    {
        return erConstant;
    }

    inline virtual ExpressionResult result()
    {
        return _value;
    }

    inline void setValue(ExpressionResultSize size, quint64 value)
    {
        _value.size = size;
        _value.result.ui64 = value;
    }

private:
    ExpressionResult _value;
};

/**
 * This expression represents a local or global variable that may or may not be
 * evaluated.
 */
class ASTVariableExpression: public ASTExpression
{
public:
    ASTVariableExpression(const ASTSymbol* symbol = 0) : _symbol(symbol) {}

    inline virtual ExpressionType type() const
    {
        return etVariable;
    }

    inline virtual int resultType() const
    {
        if (!_symbol)
            return erUndefined;
        return _symbol->isGlobal() ? erGlobalVar : erLocalVar;
    }

    virtual ExpressionResult result()
    {
        /// @todo Fixme
        return ExpressionResult(resultType(), esInt32, 0);
    }

    const ASTSymbol* symbol() const
    {
        return _symbol;
    }

protected:
    const ASTSymbol* _symbol;
};

/**
 * This expression represents any type of binary arithmetic or logical
 * expression, such as "x && y ", "x + y", or "x << y".
 */
class ASTBinaryExpression: public ASTExpression
{
public:
    ASTBinaryExpression(ExpressionType type) :
        _type(type), _left(0), _right(0) {}

    inline ASTExpression* left() const
    {
        return _left;
    }

    inline void setLeft(ASTExpression* expr)
    {
        _left = expr;
    }

    inline ASTExpression* right() const
    {
        return _right;
    }

    inline void setRight(ASTExpression* expr)
    {
        _right = expr;
    }

    inline virtual ExpressionType type() const
    {
        return _type;
    }

    inline virtual int resultType() const
    {
        return (_left && _right) ?
                    _left->resultType() | _right->resultType() :
                    erUndefined;
    }

    inline virtual ExpressionResult result()
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

        switch (_type) {
        case etLogicalOr:
            ret.size = esInt32;
            ret.result.i32 = (lr.value() || rr.value()) ? 1 : 0;
            break;

        case etLogicalAnd:
            ret.size = esInt32;
            ret.result.i32 = (lr.value() && rr.value()) ? 1 : 0;
            break;

        case etInclusiveOr:
            if (isUnsigned)
                ret.result.ui64 = lr.uvalue(target) | rr.uvalue(target);
            else
                ret.result.i64 = lr.value(target) | rr.value(target);
            break;

        case etExclusiveOr:
            if (isUnsigned)
                ret.result.ui64 = lr.uvalue(target) ^ rr.uvalue(target);
            else
                ret.result.i64 = lr.value(target) ^ rr.value(target);
            break;

        case etAnd:
            if (isUnsigned)
                ret.result.ui64 = lr.uvalue(target) & rr.uvalue(target);
            else
                ret.result.i64 = lr.value(target) & rr.value(target);
            break;

        case etEquality:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue(target) == rr.uvalue(target)) ? 1 : 0;
            else
                ret.result.i32 = (lr.value(target) == rr.value(target)) ? 1 : 0;
            break;

        case etUnequality:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue(target) != rr.uvalue(target)) ? 1 : 0;
            else
                ret.result.i32 = (lr.value(target) != rr.value(target)) ? 1 : 0;
            break;

        case etRelationalGE:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue(target) >= rr.uvalue(target)) ? 1 : 0;
            else
                ret.result.i32 = (lr.value(target) >= rr.value(target)) ? 1 : 0;
            break;

        case etRelationalGT:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue(target) > rr.uvalue(target)) ? 1 : 0;
            else
                ret.result.i32 = (lr.value(target) > rr.value(target)) ? 1 : 0;
            break;

        case etRelationalLE:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue(target) <= rr.uvalue(target)) ? 1 : 0;
            else
                ret.result.i32 = (lr.value(target) <= rr.value(target)) ? 1 : 0;
            break;

        case etRelationalLT:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue(target) < rr.uvalue(target)) ? 1 : 0;
            else
                ret.result.i32 = (lr.value(target) < rr.value(target)) ? 1 : 0;
            break;

        case etShiftLeft:
            ret.size = lr.size; // result is the type of the left hand
            // If right-hand operand is negative, the result is undefined.
            if (lr.size & esUnsigned)
                ret.result.ui64 = lr.uvalue() << rr.value();
            else
                ret.result.i64 = lr.value() << rr.value();
            break;

        case etShiftRight:
            ret.size = lr.size; // result is the type of the left hand
            // If right-hand operand is negative, the result is undefined.
            if (lr.size & esUnsigned)
                ret.result.ui64 = lr.uvalue() >> rr.value();
            else
                ret.result.i64 = lr.value() >> rr.value();
            break;

        case etAdditivePlus:
            if (isUnsigned)
                ret.result.ui64 = lr.uvalue(target) + rr.uvalue(target);
            else
                ret.result.i64 = lr.value(target) + rr.value(target);
            break;

        case etAdditiveMinus:
            if (isUnsigned)
                ret.result.ui64 = lr.uvalue(target) - rr.uvalue(target);
            else
                ret.result.i64 = lr.value(target) - rr.value(target);
            break;

        case etMultiplicativeMult:
            if (lr.size & esUnsigned) {
                if (rr.size & esUnsigned)
                    ret.result.i64 = lr.uvalue(target) * rr.uvalue(target);
                else
                    ret.result.i64 = lr.uvalue(target) * rr.value(target);
            }
            else {
                if (rr.size & esUnsigned)
                    ret.result.i64 = lr.value(target) * rr.uvalue(target);
                else
                    ret.result.i64 = lr.value(target) * rr.value(target);
            }
            break;

        case etMultiplicativeDiv:
            if (lr.size & esUnsigned) {
                if (rr.size & esUnsigned)
                    ret.result.i64 = lr.uvalue(target) / rr.uvalue(target);
                else
                    ret.result.i64 = lr.uvalue(target) / rr.value(target);
            }
            else {
                if (rr.size & esUnsigned)
                    ret.result.i64 = lr.value(target) / rr.uvalue(target);
                else
                    ret.result.i64 = lr.value(target) / rr.value(target);
            }
            break;

        case etMultiplicativeMod:
            if (lr.size & esUnsigned) {
                if (rr.size & esUnsigned)
                    ret.result.i64 = lr.uvalue(target) % rr.uvalue(target);
                else
                    ret.result.i64 = lr.uvalue(target) % rr.value(target);
            }
            else {
                if (rr.size & esUnsigned)
                    ret.result.i64 = lr.value(target) % rr.uvalue(target);
                else
                    ret.result.i64 = lr.value(target) % rr.value(target);
            }
            break;

        default:
            ret.resultType = erUndefined;
        }

        return ret;
    }

    static ExpressionResultSize binaryExprSize(const ExpressionResult& r1,
                                               const ExpressionResult& r2)
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

protected:
    ExpressionType _type;
    ASTExpression* _left;
    ASTExpression* _right;
};

/**
 * This expression represents any type of unary prefix expression, such as
 * "++i" or
 */
class ASTUnaryExpression: public ASTExpression
{
public:
    ASTUnaryExpression(ExpressionType type) :
        _type(type), _child(0) {}

    inline ASTExpression* child() const
    {
        return _child;
    }

    inline void setChild(ASTExpression* expr)
    {
        _child = expr;
    }

    inline virtual ExpressionType type() const
    {
        return _type;
    }

    inline virtual int resultType() const
    {
        return _child ? _child->resultType() : erUndefined;
    }

    inline virtual ExpressionResult result()
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

        switch (_type) {
        case etUnaryInc:
            if (res.size & es64Bit)
                ++res.result.ui64;
            else if (res.size & es32Bit)
                ++res.result.ui32;
            else if (res.size & es16Bit)
                ++res.result.ui16;
            else
                ++res.result.ui8;
            break;

        case etUnaryDec:
            if (res.size & es64Bit)
                --res.result.ui64;
            else if (res.size & es32Bit)
                --res.result.ui32;
            else if (res.size & es16Bit)
                --res.result.ui16;
            else
                --res.result.ui8;
            break;

            /// @todo Fixme
//        case etUnaryStar:
//        case etUnaryAmp:

        case etUnaryMinus:
            if (res.size & esUnsigned)
                res.result.ui64 = -res.result.ui64;
            else
                res.result.i64 = -res.result.i64;
            break;

        case etUnaryInv:
            res.result.ui64 = ~res.result.ui64;
            break;

        case etUnaryNot:
            res.size = esInt32;
            res.result.i32 = (!res.uvalue()) ? 1 : 0;
            break;

        default:
            res.resultType = erUndefined;
        }

        return res;
    }

protected:
    ExpressionType _type;
    ASTExpression* _child;
};




#endif // ASTEXPRESSION_H
