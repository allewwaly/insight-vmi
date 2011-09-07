/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include "kernelsourcetypeevaluator.h"
#include "symfactory.h"
#include <debug.h>

KernelSourceTypeEvaluator::KernelSourceTypeEvaluator(AbstractSyntaxTree* ast,
        SymFactory* factory)
    : ASTTypeEvaluator(ast, factory->memSpecs().sizeofUnsignedLong),
      _factory(factory)
{
}


KernelSourceTypeEvaluator::~KernelSourceTypeEvaluator()
{
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(const ASTNode* srcNode,
        const ASTSymbol& symbol, const ASTType* ctxType, const ASTNode* ctxNode,
        const QStringList& ctxMembers, const ASTNode* targetNode,
        const ASTType* targetType)
{
    /// @todo implement me

    // Try to find context type in the factory
    if (ctxType->type() & (rtStruct|rtUnion)) {
        if (!ctxType->identifier().isEmpty()) {
            const BaseType* bt = _factory->findBaseTypeByName(ctxType->identifier());
            if (!bt) {
                debugmsg("Did not find type \"" << ctxType->identifier()
                         << "\"");
                goto inherited;
            }
            if (bt->type() != ctxType->type()) {
                debugerr("We expected \"" << ctxType->identifier() << "\" "
                         << "to be of type " << realTypeToStr(ctxType->type())
                         << " but we found " << realTypeToStr(bt->type()));
                goto inherited;
            }
            debugmsg(QString("Found type 0x%1: %2")
                .arg(bt->id(), 0, 16)
                .arg(bt->prettyName()));

            const StructuredMember* member = 0;
            for (int i = 0; i < ctxMembers.size(); ++i) {
                debugmsg("Looking for member " << ctxMembers[i]);
                const Structured* str = dynamic_cast<const Structured*>(bt);
                if (!str) {
                    if (bt)
                        debugerr("The type " << bt->prettyName() << " is not "
                                 "a struct or union.");
                    else
                        debugerr("We have no type anymore.");
                    goto inherited;
                }
                if (!str->memberExists(ctxMembers[i])) {
                    debugerr(str->prettyName() << " has no member \""
                             << ctxMembers[i]);
                    goto inherited;
                }
                member = str->findMember(ctxMembers[i]);
                bt = member->refType();
            }


        }
        else {
            debugmsg("The following context type has no identifier:");
        }
    }
    else {
        debugmsg("Neither struct nor union:");
    }

    inherited:
    ASTTypeEvaluator::primaryExpressionTypeChange(
                srcNode, symbol, ctxType, ctxNode, ctxMembers,
                targetNode, targetType);
}

