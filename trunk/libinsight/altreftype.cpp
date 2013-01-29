#include <insight/altreftype.h>
#include <insight/astexpression.h>
#include <insight/symfactory.h>
#include <insight/pointer.h>

AltRefType::AltRefType(int id, const ASTExpression *expr)
    : _id(id), _expr(expr)
{
    updateVarExpr();
}


bool AltRefType::compatible(const Instance *inst) const
{
    if (_varExpr.isEmpty())
        return true;
    if (!inst)
        return false;
    for (int i = 0; i < _varExpr.size(); ++i) {
        if (!_varExpr[i]->compatible(inst))
            return false;
    }
    return true;
}


Instance AltRefType::toInstance(VirtualMemory *vmem, const Instance *inst,
                                const SymFactory *factory, const QString& name,
                                const QStringList& parentNames) const
{
    // Evaluate pointer arithmetic for new address
    ExpressionResult result = _expr->result(inst);
    if (!result.isValid())
        return Instance(Instance::orCandidate);

    quint64 newAddr = result.uvalue(esUInt64);
    // Retrieve new type
    const BaseType* newType = factory ?
                factory->findBaseTypeById(_id) : 0;
    assert(newType != 0);
    // Calculating the new address already corresponds to a dereference, so
    // get rid of one pointer instance
    assert(newType->type() & (rtPointer|rtArray));
    newType = dynamic_cast<const Pointer*>(newType)->refType();

    // Create instance with new type at new address
    if (newType) {
        Instance inst(newType->toInstance(newAddr, vmem, name, parentNames));
        inst.setOrigin(Instance::orCandidate);
        return inst;
    }

    return Instance(Instance::orCandidate);
}


void AltRefType::readFrom(KernelSymbolStream &in, SymFactory* factory)
{
    qint32 id;

    in >> id;
    _id = id;
    _expr = ASTExpression::fromStream(in, factory);
    updateVarExpr();
}


void AltRefType::writeTo(KernelSymbolStream &out) const
{
    out << (qint32) _id;
    ASTExpression::toStream(_expr, out);
}


void AltRefType::updateVarExpr()
{
    _varExpr.clear();
    if (!_expr)
        return;
    ASTConstExpressionList list = _expr->findExpressions(etVariable);
    for (int i = 0; i < list.size(); ++i) {
        const ASTVariableExpression* ve =
                dynamic_cast<const ASTVariableExpression*>(list[i]);
        if (ve)
            _varExpr.append(ve);
    }
}
