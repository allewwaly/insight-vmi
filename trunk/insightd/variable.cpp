/*
 * Variable.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "variable.h"
#include "basetype.h"
#include "funcpointer.h"
#include "typeruleengine.h"
#include <QScopedPointer>

#include <assert.h>

const TypeRuleEngine* Variable::_ruleEngine = 0;
static const ConstMemberList emtpyMemberList;

Variable::Variable(SymFactory* factory)
	: Symbol(factory), _offset(0)
{
}


Variable::Variable(SymFactory* factory, const TypeInfo& info)
	: Symbol(factory, info), ReferencingType(info), SourceRef(info), _offset(info.location())
{
}


QString Variable::prettyName(const QString &varName) const
{
    Q_UNUSED(varName);
    const BaseType* t = refType();
    const FuncPointer *fp = dynamic_cast<const FuncPointer*>(
                refTypeDeep(BaseType::trAnyButTypedef));

    if (fp)
        return fp->prettyName(_name, dynamic_cast<const RefBaseType*>(t));
    else if (t)
        return t->prettyName(_name);
    else
        return QString("(unresolved type 0x%1) %2").arg((uint)_refTypeId, 0, 16).arg(_name);
}


QString Variable::toString(VirtualMemory* mem) const
{
    const BaseType* t = refType();
    if (t)
        return t->toString(mem, _offset);
    else
        return QString();
}


Instance Variable::toInstance(VirtualMemory* vmem, int resolveTypes,
							  KnowledgeSources src, int *result) const
{
	Instance ret;
	int match = 0;
	if (result)
		*result = TypeRuleEngine::mrNoMatch;

	// If allowed, and available, try the rule engine first
	if (_ruleEngine && !(src & ksNoRulesEngine)) {
		ret = createRefInstance(_offset, vmem, _name, _id, resolveTypes);
		ret.setOrigin(Instance::orVariable);

		Instance *newInst = 0;
		match = _ruleEngine->match(&ret, emtpyMemberList, &newInst);
		// Auto-delete object later
		QScopedPointer<Instance> newInstPtr(newInst);

		if (result)
			*result = match;

		// Did we have a match? Was the it ambiguous or rejected?
		if (TypeRuleEngine::useMatchedInst(match)) {
			if (!newInst)
				return Instance(Instance::orRuleEngine);
			else
				ret = *newInst;
		}
	}

	// If no match or multiple matches through the rule engine, try to resolve
	// it ourself
	if ( !(match & TypeRuleEngine::mrMatch) ||
		 (match & TypeRuleEngine::mrAmbiguous) )
	{
		// If variable has exactly one alternative type and the user did
		// not explicitly request the original type, we return the
		// alternative type
		if ((src & ksNoAltTypes) || (match & TypeRuleEngine::mrAmbiguous) ||
			altRefTypeCount() != 1)
		{
			ret = createRefInstance(_offset, vmem, _name, _id, resolveTypes);
			ret.setOrigin(Instance::orVariable);
		}
		else
			ret = altRefTypeInstance(vmem, 0);
	}

	// If we need this instance for further rule evaluation, we make ret
	// a parent of itself. That way, it will be the parent of any instance
	// resulting from Instance::dereference().
	if (match & TypeRuleEngine::mrDefer)
		ret.setParent(ret, 0);

	if (result)
		*result = match;

	return ret;
}


void Variable::readFrom(KernelSymbolStream& in)
{
    Symbol::readFrom(in);
    ReferencingType::readFrom(in);
    SourceRef::readFrom(in);
    in >> _offset;
}


void Variable::writeTo(KernelSymbolStream& out) const
{
    Symbol::writeTo(out);
    ReferencingType::writeTo(out);
    SourceRef::writeTo(out);
    out << _offset;
}


KernelSymbolStream& operator>>(KernelSymbolStream& in, Variable& var)
{
    var.readFrom(in);
    return in;
}


KernelSymbolStream& operator<<(KernelSymbolStream& out, const Variable& var)
{
    var.writeTo(out);
    return out;
}


Instance Variable::altRefTypeInstance(VirtualMemory* vmem, int index) const
{
	if (index < 0 || index >= altRefTypeCount())
		return Instance();

	AltRefType alt = altRefType(index);
	Instance inst = toInstance(vmem, BaseType::trLexical, ksNone);
	return alt.toInstance(vmem, &inst, _factory, name(), QStringList());
}

