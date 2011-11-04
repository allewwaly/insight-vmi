#ifndef ASTEXPRESSION_H
#define ASTEXPRESSION_H

#include <QtGlobal>
#include <astsymbol.h>

/// Different types of expressions
enum ExpressionType {
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
    etAdditive,
    etMultiplicative
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
    erParameter = (1 << 3)   ///< Expression involves function parameters
};


/// The result of an expression
struct ExpressionResult
{
    /// Constructor
    ExpressionResult() : resultType(erUndefined) { this->result.i64 = 0; }
    ExpressionResult(int resultType) :
        resultType(resultType) { this->result.i64 = 0; }
    ExpressionResult(int resultType, quint64 result) :
        resultType(resultType) { this->result.ui64 = result; }

    /// ORed combination of ExpressionResultType values
    int resultType;

    /// Expression result, if valid
    union Result {
        quint64 ui64;
        qint64 i64;
    } result;
};

/**
  Abstract base class for a syntax tree expression.
 */
class ASTExpression
{
public:
    ASTExpression() : _alternative(0) {}

    virtual ExpressionType type() const = 0;
    virtual int resultType() = 0;
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


class ASTConstantExpression: public ASTExpression
{
public:
    ASTConstantExpression(quint64 value = 0) : _value(value) {}

    inline virtual ExpressionType type() const
    {
        return etConstant;
    }

    inline virtual int resultType()
    {
        return erConstant;
    }

    inline virtual ExpressionResult result()
    {
        return ExpressionResult(erConstant, _value);
    }

    inline quint64 value() const
    {
        return _value;
    }

private:
    quint64 _value;
};


class ASTVariableExpression: public ASTExpression
{
public:
    ASTVariableExpression(const ASTSymbol* symbol = 0) : _symbol(symbol) {}

    inline virtual ExpressionType type() const
    {
        return etVariable;
    }

    inline virtual int resultType()
    {
        if (!_symbol)
            return erUndefined;
        return _symbol->isGlobal() ? erGlobalVar : erLocalVar;
    }

    //    virtual ExpressionResult result() = 0;

    const ASTSymbol* symbol() const
    {
        return _symbol;
    }

private:
    const ASTSymbol* _symbol;
};


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

    inline virtual int resultType()
    {
        return _left->resultType() | _right->resultType();
    }

    inline virtual ExpressionResult result()
    {
        ExpressionResult lr = _left->result();
        ExpressionResult rr = _right->result();
        ExpressionResult ret(lr.resultType | rr.resultType);
        // Is the expression decidable?
        if (lr.resultType != erConstant || rr.resultType != erConstant) {
            // Undecidable, so return the correct result type
            return ret;
        }

        switch (_type) {
        case etLogicalOr:
            ret.result.ui64 = (lr.result.ui64 || rr.result.ui64) ? 1 : 0;
            break;

        case etLogicalAnd:
            ret.result.ui64 = (lr.result.ui64 && rr.result.ui64) ? 1 : 0;
            break;

        case etInclusiveOr:
            ret.result.ui64 = lr.result.ui64 | rr.result.ui64;
            break;

        case etExclusiveOr:
            ret.result.ui64 = lr.result.ui64 ^ rr.result.ui64;
            break;

        case etAnd:
            ret.result.ui64 = lr.result.ui64 & rr.result.ui64;
            break;

        case etEquality:
            ret.result.ui64 = (lr.result.ui64 == rr.result.ui64) ? 1 : 0;
            break;

        case etUnequality:
            ret.result.ui64 = (lr.result.ui64 != rr.result.ui64) ? 1 : 0;
            break;

        case etRelationalGE:
            ret.result.ui64 = (lr.result.ui64 >= rr.result.ui64) ? 1 : 0;
            break;

        case etRelationalGT:
            ret.result.ui64 = (lr.result.ui64 > rr.result.ui64) ? 1 : 0;
            break;

        case etRelationalLE:
            ret.result.ui64 = (lr.result.ui64 <= rr.result.ui64) ? 1 : 0;
            break;

        case etRelationalLT:
            ret.result.ui64 = (lr.result.ui64 < rr.result.ui64) ? 1 : 0;
            break;

        case etShiftLeft:
            ret.result.ui64 = lr.result.ui64 << rr.result.ui64;
            break;

        case etShiftRight:
            ret.result.ui64 = lr.result.ui64 >> rr.result.ui64;
            break;

        case etAdditive:
            ret.result.ui64 = lr.result.ui64 + rr.result.ui64;
            break;

        case etMultiplicative:
            /// @todo What about signedness here???
            ret.result.ui64 = lr.result.ui64 * rr.result.ui64;
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


#endif // ASTEXPRESSION_H
