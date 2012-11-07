#ifndef ALTREFTYPE_H
#define ALTREFTYPE_H

#include <QStringList>
#include "instance_def.h"
#include "kernelsymbolstream.h"

class SymFactory;
class ASTExpression;
class ASTVariableExpression;


class AltRefType
{
public:
    /**
     * Constructor
     * @param id the ID of the target BaseType
     * @param expr the ASTExpression to compute the target type's address
     */
    explicit AltRefType(int id = -1, const ASTExpression* expr = 0);

    /**
     * Checks if the given Instance \a inst is compatible with the
     * expression that needs to be evaluated for this alternative type.
     * Therefore all ASTVariableExpressions within the expression are
     * checked for type compatibility with \a inst.
     * @param inst the Instance object to check against the expression
     * @return \c true if \a inst is compatible to the expression, \c false
     * otherwise
     */
    bool compatible(const Instance *inst) const;

    /**
     * Creates an Instance of this alternative type.
     * @param vmem the VirtualMemory object to create the instance for
     * @param inst the Instance object to evaluate expr with
     * @param factory the factory to which this type belongs
     * @param name the name for this instance, i.e., variable or member name
     * @param parentNames the parent name components used to reach this
     * alternative type
     */
    Instance toInstance(VirtualMemory* vmem, const Instance* inst,
                        const SymFactory* factory, const QString &name,
                        const QStringList &parentNames) const;

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     * @param factory the factory holding the symbols
     */
    void readFrom(KernelSymbolStream& in, SymFactory *factory);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    void writeTo(KernelSymbolStream &out) const;

    /**
     * Return the ID of the target BaseType
     */
    inline int id() const { return _id; }

    /**
     * Sets the ID of the target BaseType
     * @param id new type ID
     */
    inline void setId(int id) { _id = id; }

    /**
     * Returns the ASTExpression to compute the target type's address
     */
    inline const ASTExpression* expr() const { return _expr; }

    /**
     * Sets the ASTExpression to compute the target type's address.
     * @param expr new expression
     */
    inline void setExpr(const ASTExpression* expr)
    {
        _expr = expr;
        updateVarExpr();
    }

private:
    void updateVarExpr();
    int _id;
    const ASTExpression* _expr;
    QList<const ASTVariableExpression*> _varExpr;
};

#endif // ALTREFTYPE_H
