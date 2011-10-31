#ifndef ASTEXPRESSION_H
#define ASTEXPRESSION_H

#include <QtGlobal>
#include <astsymbol.h>

/// Different types of expressions
enum ExpressionType {
    etConstant,
    etVariable
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
    ExpressionResult() : resultType(erUndefined) { result.i64 = 0; }

    /// ORed combination of ExpressionResultType values
    int resultType;

    /// Expression result, if valid
    union {
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
    virtual ExpressionType type() const = 0;
    virtual int resultType() = 0;
//    virtual ExpressionResult result() = 0;
};


class ASTConstantExpression
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

    //    virtual ExpressionResult result() = 0;

    inline quint64 value() const
    {
        return _value;
    }

private:
    quint64 _value;
};


class ASTVariableExpression
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


#endif // ASTEXPRESSION_H
