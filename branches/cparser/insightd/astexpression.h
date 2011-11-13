#ifndef ASTEXPRESSION_H
#define ASTEXPRESSION_H

#include <QtGlobal>
#include <astsymbol.h>
#include <debug.h>

/// Different types of expressions
enum ExpressionType {
    etVoid,
    etRuntimeDependent,
    etLiteralConstant,
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
    esUInt64    = es64Bit|esUnsigned,
    esInteger   = es8Bit|es16Bit|es32Bit|es64Bit,
    esReal      = esFloat|esDouble
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
    ExpressionResult(int resultType, ExpressionResultSize size, float result)
        : resultType(resultType), size(size) { this->result.f = result; }
    ExpressionResult(int resultType, ExpressionResultSize size, double result)
        : resultType(resultType), size(size) { this->result.d = result; }

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

    qint64 value(ExpressionResultSize target = esUndefined) const;

    quint64 uvalue(ExpressionResultSize target = esUndefined) const;

    float fvalue() const;

    double dvalue() const;
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
    virtual ExpressionResult result() const = 0;

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

    inline virtual ExpressionResult result() const
    {
        return ExpressionResult(resultType(), esInt32, 0ULL);
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

    inline virtual ExpressionResult result() const
    {
        return ExpressionResult(resultType(), esInt32, 0ULL);
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
    ASTConstantExpression(ExpressionResultSize size, float value)
        : _value(erConstant, size, value) {}
    ASTConstantExpression(ExpressionResultSize size, double value)
        : _value(erConstant, size, value) {}

    inline virtual ExpressionType type() const
    {
        return etLiteralConstant;
    }

    inline virtual int resultType() const
    {
        return erConstant;
    }

    inline virtual ExpressionResult result() const
    {
        return _value;
    }

    inline void setValue(ExpressionResultSize size, quint64 value)
    {
        _value.size = size;
        _value.result.ui64 = value;
    }

    inline void setValue(ExpressionResultSize size, float value)
    {
        _value.size = size;
        _value.result.f = value;
    }

    inline void setValue(ExpressionResultSize size, double value)
    {
        _value.size = size;
        _value.result.d = value;
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

    virtual ExpressionResult result() const
    {
        /// @todo Fixme
        return ExpressionResult(resultType(), esInt32, 0ULL);
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

    virtual ExpressionResult result() const;

    static ExpressionResultSize binaryExprSize(const ExpressionResult& r1,
                                               const ExpressionResult& r2);

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

    virtual ExpressionResult result() const;

protected:
    ExpressionType _type;
    ASTExpression* _child;
};




#endif // ASTEXPRESSION_H
