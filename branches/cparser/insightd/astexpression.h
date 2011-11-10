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
    es64Bit     = (1 << 1),
    esUnsigned  = (1 << 2),
    esInt32     = 0,
    esUInt32    = esUnsigned,
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
    ExpressionResult(int resultType, int size, quint64 result)
        : resultType(resultType), size(size) { this->result.ui64 = result; }

    /// ORed combination of ExpressionResultType values
    int resultType;

    /// size of result of expression. \sa ExpressionResultSize
    int size;

    /// Expression result, if valid
    union Result {
        quint64 ui64;
        qint64 i64;
        quint32 ui32;
        qint32 i32;
    } result;

    /// Expands the value according to signdness and size
    inline qint64 valueExpanded() const
    {
        return (size & es64Bit) ?
                    result.i64 :
                    (size & esUnsigned) ?
                        (qint64)((quint64)result.ui32) :
                        (qint64) result.i32;
    }

    inline qint64 value() const
    {
        return (size & es64Bit) ? result.i64 : (qint64) result.i32;
    }

    inline quint64 uvalue() const
    {
        return (size & es64Bit) ? result.ui64 : (quint64) result.ui32;
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
    ASTConstantExpression(int size, quint64 value)
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

    inline void setValue(int size, quint64 value)
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
        // Unsigned flag is dominant over signed flag and 64bit is dominant
        // over 32 bit, so just OR the sizes
        ret.size = lr.size | rr.size;
        // Is the expression decidable?
        if (lr.resultType != erConstant || rr.resultType != erConstant) {
            // Undecidable, so return the combined result type
            return ret;
        }

//        bool is64Bit = ((lr.size|rr.size) & es64Bit) ? true : false;
        bool isUnsigned = ((lr.size|rr.size) & esUnsigned) ? true : false;

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
            ret.result.i64 = lr.valueExpanded() | rr.valueExpanded();
            break;

        case etExclusiveOr:
            ret.result.i64 = lr.valueExpanded() ^ rr.valueExpanded();
            break;

        case etAnd:
            ret.result.i64 = lr.valueExpanded() & rr.valueExpanded();
            break;

        case etEquality:
            ret.size = esInt32;
            ret.result.i32 = (lr.value() == rr.value()) ? 1 : 0;
            break;

        case etUnequality:
            ret.size = esInt32;
            ret.result.i32 = (lr.value() != rr.value()) ? 1 : 0;
            break;

        case etRelationalGE:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue() >= rr.uvalue()) ? 1 : 0;
            else
                ret.result.i32 = (lr.value() >= rr.value()) ? 1 : 0;
            break;

        case etRelationalGT:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue() > rr.uvalue()) ? 1 : 0;
            else
                ret.result.i32 = (lr.value() > rr.value()) ? 1 : 0;
            break;

        case etRelationalLE:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue() <= rr.uvalue()) ? 1 : 0;
            else
                ret.result.i32 = (lr.value() <= rr.value()) ? 1 : 0;
            break;

        case etRelationalLT:
            ret.size = esInt32;
            if (isUnsigned)
                ret.result.i32 = (lr.uvalue() < rr.uvalue()) ? 1 : 0;
            else
                ret.result.i32 = (lr.value() < rr.value()) ? 1 : 0;
            break;

        case etShiftLeft:
            ret.size = lr.size;
            if (lr.size & es64Bit) {
                if (lr.size & esUnsigned) {
                    if (rr.size & esUnsigned)
                        ret.result.ui64 = lr.uvalue() << rr.uvalue();
                    else
                        ret.result.ui64 = lr.uvalue() << rr.value();
                }
                else {
                    if (rr.size & esUnsigned)
                        ret.result.i64 = lr.value() << rr.uvalue();
                    else
                        ret.result.i64 = lr.value() << rr.value();
                }
            }
            else {
                if (lr.size & esUnsigned) {
                    if (rr.size & esUnsigned)
                        ret.result.ui32 = lr.uvalue() << rr.uvalue();
                    else
                        ret.result.ui32 = lr.uvalue() << rr.value();
                }
                else {
                    if (rr.size & esUnsigned)
                        ret.result.i32 = lr.value() << rr.uvalue();
                    else
                        ret.result.i32 = lr.value() << rr.value();
                }
            }
            break;

        case etShiftRight:
            ret.size = lr.size;
            if (lr.size & es64Bit) {
                if (lr.size & esUnsigned) {
                    if (rr.size & esUnsigned)
                        ret.result.ui64 = lr.uvalue() >> rr.uvalue();
                    else
                        ret.result.ui64 = lr.uvalue() >> rr.value();
                }
                else {
                    if (rr.size & esUnsigned)
                        ret.result.i64 = lr.value() >> rr.uvalue();
                    else
                        ret.result.i64 = lr.value() >> rr.value();
                }
            }
            else {
                if (lr.size & esUnsigned) {
                    if (rr.size & esUnsigned)
                        ret.result.ui32 = lr.uvalue() >> rr.uvalue();
                    else
                        ret.result.ui32 = lr.uvalue() >> rr.value();
                }
                else {
                    if (rr.size & esUnsigned)
                        ret.result.i32 = lr.value() >> rr.uvalue();
                    else
                        ret.result.i32 = lr.value() >> rr.value();
                }
            }
            break;

        case etAdditivePlus:
            if (isUnsigned)
                ret.result.ui64 = lr.result.ui64 + rr.result.ui64;
            else
                ret.result.i64 = lr.result.i64 + rr.result.i64;
            break;

        case etAdditiveMinus:
            if (isUnsigned)
                ret.result.ui64 = lr.result.ui64 - rr.result.ui64;
            else
                ret.result.i64 = lr.result.i64 - rr.result.i64;
            break;

        case etMultiplicativeMult:
            if (isUnsigned)
                ret.result.ui64 = lr.result.ui64 * rr.result.ui64;
            else
                ret.result.i64 = lr.result.i64 * rr.result.i64;
            break;

        case etMultiplicativeDiv:
            if (isUnsigned)
                ret.result.ui64 = lr.result.ui64 / rr.result.ui64;
            else
                ret.result.i64 = lr.result.i64 / rr.result.i64;
            break;

        default:
            ret.resultType = erUndefined;
        }

        return ret;
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
            // Undecidable, so return the correct result type
            res.resultType |= erInvalid;
            return res;
        }

        switch (_type) {
        case etUnaryInc:
            if (res.size & es64Bit)
                ++res.result.ui64;
            else
                ++res.result.ui32;
            break;

        case etUnaryDec:
            if (res.size & es64Bit)
                --res.result.ui64;
            else
                --res.result.ui32;
            break;

            /// @todo Fixme
//        case etUnaryStar:
//        case etUnaryAmp:

        case etUnaryMinus:
            if (res.size & es64Bit)
                res.result.i64  = -res.result.i64;
            else
                res.result.i32 = -res.result.i32;
            break;

        case etUnaryInv:
            if (res.size & es64Bit)
                res.result.ui64  = ~res.result.ui64;
            else
                res.result.ui32 = ~res.result.ui32;
            break;

        case etUnaryNot:
            res.size = esInt32;
            res.result.i32 = !res.value() ? 1 : 0;
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
