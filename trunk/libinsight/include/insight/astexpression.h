#ifndef ASTEXPRESSION_H
#define ASTEXPRESSION_H

#include <QList>
#include <astsymbol.h>
#include <debug.h>
#include <safeflags.h>
#include "expressionresult.h"
#include "kernelsymbolstream.h"

class BaseType;
class ASTExpression;
class Instance;
class SymFactory;

typedef QList<ASTExpression*> ASTExpressionList;
typedef QList<const ASTExpression*> ASTConstExpressionList;


/// Different types of expressions
enum ExpressionType {
    etNull = 0,
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
DECLARE_SAFE_FLAGS(ExpressionTypes, ExpressionType)

const char* expressionTypeToString(ExpressionTypes type);

/**
 * Abstract base class for a syntax tree expression.
 */
class ASTExpression
{
    friend class ASTBinaryExpression;
    friend class ASTUnaryExpression;
public:
    ASTExpression() : _alternative(0) {}

    virtual ~ASTExpression(){}
    virtual ExpressionType type() const = 0;
    virtual ExprResultTypes resultType() const = 0;
    virtual ExpressionResult result(const Instance* inst = 0) const = 0;
    virtual ASTExpression* copy(ASTExpressionList& list,
                                bool recursive = true) const = 0;
    virtual ASTExpression* copy(ASTConstExpressionList& list,
                                bool recursive = true) const = 0;

    inline ASTExpressionList flaten()
    {
        ASTExpressionList list;
        flaten(list);
        return list;
    }

    inline ASTConstExpressionList flaten() const
    {
        ASTConstExpressionList list;
        flaten(list);
        return list;
    }

    virtual ASTConstExpressionList expandAlternatives(ASTConstExpressionList &list) const;
    virtual QString toString(bool compact = false) const = 0;

    inline virtual bool equals(const ASTExpression* other) const
    {
        return other && type() == other->type();
    }

    inline ASTExpressionList findExpressions(ExpressionTypes type)
    {
        ASTExpressionList list;
        this->findExpressions(type, &list);
        return list;
    }

    inline ASTConstExpressionList findExpressions(ExpressionTypes type)
        const
    {
        ASTConstExpressionList list;
        this->findExpressions(type, &list);
        return list;
    }

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

    virtual void readFrom(KernelSymbolStream &in, SymFactory* factory);
    virtual void writeTo(KernelSymbolStream &out) const;

    static ASTExpression* fromStream(KernelSymbolStream &in,
                                     SymFactory* factory);

    static void toStream(const ASTExpression *expr, KernelSymbolStream &out);

protected:
    template<class T, class list_t>
    T* copyTempl(list_t& list, bool recursive) const
    {
        T* expr = new T(*dynamic_cast<const T*>(this));
        list.append(expr);
        if (recursive && _alternative)
            expr->_alternative = _alternative->copy(list, recursive);
        return expr;
    }

    inline virtual void flaten(ASTExpressionList& list)
    {
        list += this;
    }

    inline virtual void flaten(ASTConstExpressionList& list) const
    {
        list += this;
    }

    inline virtual void findExpressions(ExpressionTypes type,
                                        ASTExpressionList *list)
    {
        if (type == this->type())
            list->append(this);
        if (_alternative)
            _alternative->findExpressions(type, list);
    }

    inline virtual void findExpressions(ExpressionTypes type,
                                        ASTConstExpressionList *list) const
    {
        if (type == this->type())
            list->append(this);
        if (_alternative)
            _alternative->findExpressions(type, list);
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

    inline virtual ExprResultTypes resultType() const
    {
        return erUndefined;
    }

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return ExpressionResult(resultType(), esInt32, 0ULL);
    }

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTUndefinedExpression>(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                             bool recursive = true) const
    {
        return copyTempl<ASTUndefinedExpression>(list, recursive);
    }

    inline virtual QString toString(bool compact = false) const
    {
        Q_UNUSED(compact);
        return "(undefined expr.)";
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

    inline virtual ExprResultTypes resultType() const
    {
        return erUndefined;
    }

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return ExpressionResult(resultType(), esInt32, 0ULL);
    }

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTVoidExpression>(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTVoidExpression>(list, recursive);
    }

    inline virtual QString toString(bool compact = false) const
    {
        Q_UNUSED(compact);
        return "(void)";
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

    inline virtual ExprResultTypes resultType() const
    {
        return erRuntime;
    }

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return ExpressionResult(resultType(), esInt32, 0ULL);
    }

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTRuntimeExpression>(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTRuntimeExpression>(list, recursive);
    }

    inline virtual QString toString(bool compact = false) const
    {
        Q_UNUSED(compact);
        return "(runtime expr.)";
    }
};

/**
 * This expression represents a compile-time constant value.
 */
class ASTConstantExpression: public ASTExpression
{
public:
    ASTConstantExpression() {}
    ASTConstantExpression(ExprResultSizes size, quint64 value)
        : _value(erConstant, size, value) {}
    ASTConstantExpression(ExprResultSizes size, float value)
        : _value(erConstant, size, value) {}
    ASTConstantExpression(ExprResultSizes size, double value)
        : _value(erConstant, size, value) {}

    inline virtual ExpressionType type() const
    {
        return etLiteralConstant;
    }

    inline virtual ExprResultTypes resultType() const
    {
        return erConstant;
    }

    inline virtual ExpressionResult result(const Instance* inst = 0) const
    {
        Q_UNUSED(inst);
        return _value;
    }

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTConstantExpression>(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        return copyTempl<ASTConstantExpression>(list, recursive);
    }

    inline virtual QString toString(bool compact = false) const
    {
        Q_UNUSED(compact);
        return _value.toString();
    }

    virtual bool equals(const ASTExpression* other) const
    {
        if (!ASTExpression::equals(other))
            return false;
        const ASTConstantExpression* c =
                dynamic_cast<const ASTConstantExpression*>(other);
        return c && _value.uvalue() == c->_value.uvalue();
    }

    inline void setValue(ExprResultSizes size, quint64 value)
    {
        _value.size = size;
        _value.result.ui64 = value;
    }

    inline void setValue(ExprResultSizes size, float value)
    {
        _value.size = size;
        _value.result.f = value;
    }

    inline void setValue(ExprResultSizes size, double value)
    {
        _value.size = size;
        _value.result.d = value;
    }

    virtual void readFrom(KernelSymbolStream &in, SymFactory* factory);
    virtual void writeTo(KernelSymbolStream &out) const;

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

    inline virtual ExprResultTypes resultType() const
    {
        return _valueSet ? erConstant : erUndefined;
    }

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        ASTEnumeratorExpression* expr =
                copyTempl<ASTEnumeratorExpression>(list, recursive);
        expr->_symbol = 0;
        return expr;
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        ASTEnumeratorExpression* expr =
                copyTempl<ASTEnumeratorExpression>(list, recursive);
        expr->_symbol = 0;
        return expr;
    }

    const ASTSymbol* symbol() const
    {
        return _symbol;
    }

    virtual void readFrom(KernelSymbolStream &in, SymFactory* factory);
    virtual void writeTo(KernelSymbolStream &out) const;

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
    explicit ASTVariableExpression(const BaseType* type = 0, bool global = false)
        : _baseType(type), _global(global), _transformations(0) {}

    inline virtual ExpressionType type() const
    {
        return etVariable;
    }

    inline virtual ExprResultTypes resultType() const
    {
        if (!_baseType)
            return erUndefined;
        return _global ? erGlobalVar : erLocalVar;
    }

    virtual QString toString(bool compact = false) const;

    virtual bool equals(const ASTExpression* other) const;

    virtual bool compatible(const Instance* inst) const;

    virtual ExpressionResult result(const Instance* inst = 0) const;

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyVarTempl(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        return copyVarTempl(list, recursive);
    }

    inline const BaseType* baseType() const
    {
        return _baseType;
    }

    void appendTransformation(SymbolTransformationType type);
    void appendTransformation(const QString& member);
    void appendTransformation(int arrayIndex);

    inline const SymbolTransformations& transformations() const
    {
        return _transformations;
    }

    virtual void readFrom(KernelSymbolStream &in, SymFactory* factory);
    virtual void writeTo(KernelSymbolStream &out) const;

protected:
    template<class list_t>
    ASTExpression* copyVarTempl(list_t& list, bool recursive) const;
    const BaseType* _baseType;
    bool _global;
    SymbolTransformations _transformations;
};

/**
 * This expression represents any type of binary arithmetic or logical
 * expression, such as "x && y ", "x + y", or "x << y".
 */
class ASTBinaryExpression: public ASTExpression
{
public:
    explicit ASTBinaryExpression(ExpressionType type) :
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

    inline virtual ExprResultTypes resultType() const
    {
        return (_left && _right) ?
                    _left->resultType() | _right->resultType() :
                    ExprResultTypes(erUndefined);
    }

    virtual ExpressionResult result(const Instance* inst = 0) const;

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyBinaryTempl(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        return copyBinaryTempl(list, recursive);
    }

    virtual ASTConstExpressionList expandAlternatives(ASTConstExpressionList& list) const;

    virtual QString toString(bool compact) const;

    virtual bool equals(const ASTExpression* other) const
    {
        if (!ASTExpression::equals(other))
            return false;
        const ASTBinaryExpression* b =
                dynamic_cast<const ASTBinaryExpression*>(other);
        // Compare childen to each other
        return b && ((!_left && !b->_left) || _left->equals(b->_left)) &&
                ((!_right && !b->_right) || _right->equals(b->_right));
    }

    static ExprResultSizes binaryExprSize(const ExpressionResult& r1,
                                               const ExpressionResult& r2);

    virtual void readFrom(KernelSymbolStream &in, SymFactory* factory);
    virtual void writeTo(KernelSymbolStream &out) const;

protected:
    template<class list_t>
    inline ASTExpression* copyBinaryTempl(list_t& list, bool recursive) const
    {
        ASTBinaryExpression* expr = copyTempl<ASTBinaryExpression>(list, recursive);
        if (recursive && _left)
            expr->_left = _left->copy(list, recursive);
        if (recursive && _right)
            expr->_right = _right->copy(list, recursive);
        return expr;
    }

    inline virtual void flaten(ASTExpressionList& list)
    {
        ASTExpression::flaten(list);
        if (_left)
            _left->flaten(list);
        if (_right)
            _right->flaten(list);
    }

    inline virtual void flaten(ASTConstExpressionList& list) const
    {
        ASTExpression::flaten(list);
        if (_left)
            _left->flaten(list);
        if (_right)
            _right->flaten(list);
    }

    inline virtual void findExpressions(ExpressionTypes type,
                                        ASTExpressionList* list)
    {
        ASTExpression::findExpressions(type, list);
        if (_left)
            _left->findExpressions(type, list);
        if (_right)
            _right->findExpressions(type, list);
    }


    inline virtual void findExpressions(ExpressionTypes type,
                                        ASTConstExpressionList* list) const
    {
        ASTExpression::findExpressions(type, list);
        if (_left)
            _left->findExpressions(type, list);
        if (_right)
            _right->findExpressions(type, list);
    }

    QString operatorToString() const;

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
    explicit ASTUnaryExpression(ExpressionType type) :
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

    inline virtual ExprResultTypes resultType() const
    {
        return _child ? _child->resultType() : ExprResultTypes(erUndefined);
    }

    virtual ExpressionResult result(const Instance* inst = 0) const;

    inline virtual ASTExpression* copy(ASTExpressionList& list,
                                       bool recursive = true) const
    {
        return copyUnaryTempl(list, recursive);
    }

    inline virtual ASTExpression* copy(ASTConstExpressionList& list,
                                       bool recursive = true) const
    {
        return copyUnaryTempl(list, recursive);
    }

    virtual ASTConstExpressionList expandAlternatives(ASTConstExpressionList& list) const;

    virtual QString toString(bool compact) const;

    virtual bool equals(const ASTExpression* other) const
    {
        if (!ASTExpression::equals(other))
            return false;
        const ASTUnaryExpression* u =
                dynamic_cast<const ASTUnaryExpression*>(other);
        // Compare childen to each other
        return u && ((!_child && !u->_child) || _child->equals(u->_child));
    }

    virtual void readFrom(KernelSymbolStream &in, SymFactory* factory);
    virtual void writeTo(KernelSymbolStream &out) const;

protected:
    template<class list_t>
    ASTExpression* copyUnaryTempl(list_t& list, bool recursive) const;

    inline void flaten(ASTExpressionList& list)
    {
        ASTExpression::flaten(list);
        if (_child)
            _child->flaten(list);
    }

    inline void flaten(ASTConstExpressionList& list) const
    {
        ASTExpression::flaten(list);
        if (_child)
            _child->flaten(list);
    }

    inline virtual void findExpressions(ExpressionTypes type,
                                        ASTExpressionList* list)
    {
        ASTExpression::findExpressions(type, list);
        if (_child)
            _child->findExpressions(type, list);
    }

    inline virtual void findExpressions(ExpressionTypes type,
                                        ASTConstExpressionList* list) const
    {
        ASTExpression::findExpressions(type, list);
        if (_child)
            _child->findExpressions(type, list);
    }

    QString operatorToString() const;

    ExpressionType _type;
    ASTExpression* _child;
};


#endif // ASTEXPRESSION_H
