/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include "kernelsourcetypeevaluator.h"
#include "symfactory.h"
#include <debug.h>
#include <astnode.h>
#include <astscopemanager.h>

KernelSourceTypeEvaluator::KernelSourceTypeEvaluator(AbstractSyntaxTree* ast,
        SymFactory* factory)
    : ASTTypeEvaluator(ast, factory->memSpecs().sizeofUnsignedLong),
      _factory(factory)
{
}


KernelSourceTypeEvaluator::~KernelSourceTypeEvaluator()
{
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(
        const ASTNode* srcNode, const ASTType* srcType,
        const ASTSymbol& srcSymbol, const ASTType* ctxType,
        const ASTNode* ctxNode, const QStringList& ctxMembers,
        const ASTNode* targetNode, const ASTType* targetType)
{
    /// @todo implement me

    QList<BaseType*> srcBaseTypes;

    // Find the first non-pointer ASTType
    const ASTType* ctxTypeNonPtr = ctxType;
    while (ctxTypeNonPtr && ctxTypeNonPtr->type() == rtPointer)
        ctxTypeNonPtr = ctxTypeNonPtr->next();

    if (!ctxTypeNonPtr) {
        debugerr("We have no type besides pointers???");
        goto inherited;
    }

    // Is the source a struct or union?
    if (ctxTypeNonPtr->type() & (rtStruct|rtUnion)) {
        if (ctxTypeNonPtr->identifier().isEmpty()) {
            debugmsg("The following context type has no identifier:");
            goto inherited;
        }
        else {
            srcBaseTypes =
                    _factory->typesByName().values(ctxTypeNonPtr->identifier());
        }
    }
    // Is the source a numeric type?
    else if (ctxTypeNonPtr->type() & (~rtEnum & NumericTypes)) {
        srcBaseTypes += _factory->getNumericInstance(ctxTypeNonPtr->type());
    }



    // Try to find context type in the factory
    if (ctxType->type() & (rtStruct|rtUnion)) {
        if (!ctxType->identifier().isEmpty()) {
            debugmsg("Looking for type " << ctxType->toString());

            bool globalSymbol = (srcSymbol.astNode()->scope->parent() == 0);

            // Get all types with this name
            srcBaseTypes = _factory->typesByName().values(ctxType->identifier());

            if (srcBaseTypes.isEmpty()) {
                debugmsg("Did not find type \"" << ctxType->identifier()
                         << "\"");
                goto inherited;
            }

            // Try to find the target type

            for (int i = 0; i < srcBaseTypes.size(); ++i) {
                BaseType* bt = srcBaseTypes[i];

                if (bt->type() != ctxType->type()) {
                    debugerr("We expected \"" << ctxType->identifier() << "\" "
                             << "to be of type " << realTypeToStr(ctxType->type())
                             << " but we found " << realTypeToStr(bt->type()));
                    goto inherited;
                }
                debugmsg(QString("Found type 0x%1: %2")
                         .arg(bt->id(), 0, 16)
                         .arg(bt->prettyName()));

//                _factory->addTypeAlternativeUse(bt, ctxMembers)
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
                srcNode, srcType, srcSymbol, ctxType, ctxNode, ctxMembers,
                targetNode, targetType);
}

