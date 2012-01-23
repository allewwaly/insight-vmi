#ifndef ASTEXPRESSION_H
#define ASTEXPRESSION_H

#include <QList>
#include <astsymbol.h>
#include <debug.h>
#include "expressionresult.h"

class BaseType;
class ASTExpression;
class Instance;

typedef QList<ASTExpression*> ASTExpressionList;


/// Different types of expressions
enum ExpressionType {
    etVoid,
    etUndefined,
    etRuntimeDependent,
    etLiteralConstant,
    etEnumerator,
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
 * Abstract base class for a syntax tree expression.
 */
class ASTExpression
{
public:
    ASTExpression() : _alternative(0) {}

    virtual ExpressionType type() const = 0;
    virtual int resultType() const = 0;
    virtual ExpressionResult result(const Instance* inst = 0) const = 0;
    virtual ASTExpression* clone(ASTExpressionList& list) const = 0;

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
    template<class T>
    T* cloneTempl(ASTExpressionList& list) const
    {
        T* expr = new T(*dynamic_cast<const T*>(this));
        list.append(expr);
        if (_alternative)
            expr->_alternative = _alternative->clone(list);
        return expr;
    }


    ASTExpression* _alternative;
};

/**
 * This expression returns cannot be evaluated and returns an undefined result.
 */
class ASTUndefinedExpression: public ASTExpression
{
public:
    inline virtual ExpressionType type() const
    {
        return etUndefined;
    }

    inline virtual int resultType() const
    {
        return erUndefined;
    }

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return ExpressionResult(resultType(), esInt32, 0ULL);
    }

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        return cloneTempl<ASTUndefinedExpression>(list);
    }
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

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return ExpressionResult(resultType(), esInt32, 0ULL);
    }

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        return cloneTempl<ASTVoidExpression>(list);
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

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return ExpressionResult(resultType(), esInt32, 0ULL);
    }

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        return cloneTempl<ASTRuntimeExpression>(list);
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

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return _value;
    }

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        return cloneTempl<ASTConstantExpression>(list);
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

protected:
    ExpressionResult _value;
};

/**
 * This expression represents an enumeration value.
 */
class ASTEnumeratorExpression: public ASTConstantExpression
{
public:
    ASTEnumeratorExpression() : _symbol(0), _valueSet(false) {}
    ASTEnumeratorExpression(qint32 value, const ASTSymbol* symbol)
        : _symbol(symbol), _valueSet(true)
    {
        _value.resultType = resultType();
        _value.size = esInt32;
        _value.result.i32 = value;
    }

    inline virtual ExpressionType type() const
    {
        return etEnumerator;
    }

    inline virtual int resultType() const
    {
        return _valueSet ? erConstant : erUndefined;
    }

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        ASTEnumeratorExpression* expr =
                cloneTempl<ASTEnumeratorExpression>(list);
        expr->_symbol = 0;
        return expr;
    }

    const ASTSymbol* symbol() const
    {
        return _symbol;
    }

protected:
    const ASTSymbol* _symbol;
    bool _valueSet;
};

/**
 * This expression represents a local or global variable that may or may not be
 * evaluated.
 */
class ASTVariableExpression: public ASTExpression
{
public:
    enum PostfixExpressionType {
        ptDot,
        ptArrow,
        ptBrackets
    };

    struct PostfixExpression {
        PostfixExpression(PostfixExpressionType type, const QString& member)
            : type(type), member(member), arrayIndex(-1) {}
        PostfixExpression(int arrayIndex)
            : type(ptBrackets), arrayIndex(arrayIndex) {}
        PostfixExpressionType type;
        QString member;
        int arrayIndex;
    };

    typedef QList<PostfixExpression> PostfixExpressionList;

    ASTVariableExpression(const BaseType* type = 0, bool global = false)
        : _baseType(type), _global(global) {}

    inline virtual ExpressionType type() const
    {
        return etVariable;
    }

    inline virtual int resultType() const
    {
        if (!_baseType)
            return erUndefined;
        return _global ? erGlobalVar : erLocalVar;
    }

    virtual ExpressionResult result(const Instance* inst = 0) const;

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        return cloneTempl<ASTVariableExpression>(list);
    }

    const BaseType* baseType() const
    {
        return _baseType;
    }

    void appendPostfixExpression(PostfixExpression pe)
    {
        _pel.append(pe);
    }

protected:
    const BaseType* _baseType;
    bool _global;
    PostfixExpressionList _pel;
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

    virtual ExpressionResult result(const Instance* inst = 0) const;

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        ASTBinaryExpression* expr = cloneTempl<ASTBinaryExpression>(list);
        if (_left)
            expr->_left = _left->clone(list);
        if (_right)
            expr->_right = _right->clone(list);
        return expr;
    }


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

    virtual ExpressionResult result(const Instance* inst = 0) const;

    virtual ASTExpression* clone(ASTExpressionList& list) const
    {
        ASTUnaryExpression* expr = cloneTempl<ASTUnaryExpression>(list);
        if (_child)
            expr->_child = _child->clone(list);
        return expr;
    }

protected:
    ExpressionType _type;
    ASTExpression* _child;
};




#endif // ASTEXPRESSION_H