/*
 * astsymbol.cpp
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#include <astsymboltypes.h>
#include <astsymbol.h>
#include <astnode.h>
#include <astscopemanager.h>
#include <abstractsyntaxtree.h>
#include <asttypeevaluator.h>
#include <bitop.h>


const char* SymbolTransformation::typeToString(SymbolTransformationType type)
{
	switch (type) {
	case ttMember:		return "Member";
	case ttArray:		return "Array";
	case ttFuncCall:    return "FuncCall";
	case ttDereference: return "Dereference";
	case ttAddress:     return "Address";
	}
	return "(unknown)";
}


QString SymbolTransformation::typeString() const
{
	return typeToString(type);
}


SymbolTransformations::SymbolTransformations(ASTTypeEvaluator *typeEval)
	: _typeEval(typeEval)
{
}


void SymbolTransformations::append(const SymbolTransformation &st)
{
    // Try to simplify transformations: merge address operators and dereferences
    if (!isEmpty()) {
        if ((st.type == ttDereference && last().type == ttAddress) ||
            (st.type == ttAddress && last().type == ttDereference))
        {
            // Both operations cancel each other out
            pop_back();
            return;
        }
    }

    // No simplification, so append the transformation
    QList<SymbolTransformation>::append(st);
}


void SymbolTransformations::append(SymbolTransformationType type)
{
    SymbolTransformations::append(SymbolTransformation(type));
}


void SymbolTransformations::append(const QString &member)
{
    SymbolTransformations::append(SymbolTransformation(member));
}


void SymbolTransformations::append(int arrayIndex)
{
    SymbolTransformations::append(SymbolTransformation(arrayIndex));
}


void SymbolTransformations::append(const ASTNodeList *suffixList)
{
    if (!suffixList)
        return;

    // Append all postfix expression suffixes
    for (; suffixList; suffixList = suffixList->next)
    {
        const ASTNode* p = suffixList->item;
        switch(p->type) {
        case nt_postfix_expression_arrow:
            SymbolTransformations::append(SymbolTransformation(ttDereference));
            // no break

        case nt_postfix_expression_dot:
            SymbolTransformations::append(
                        SymbolTransformation(
                            antlrTokenToStr(p->u.postfix_expression_suffix.identifier)));
            break;

        case nt_postfix_expression_brackets: {
            // We expect array indices to be constant
            const ASTNode* e = p->u.postfix_expression_suffix.expression ?
                        p->u.postfix_expression_suffix.expression->item : 0;
            bool ok = false;
            int index = _typeEval ?
                        _typeEval->evaluateIntExpression(e, &ok) : -1;
            SymbolTransformations::append(
                        SymbolTransformation(ok ? index : -1));
            break;
        }

        case nt_postfix_expression_parens:
            SymbolTransformations::append(SymbolTransformation(ttFuncCall));
            break;

        default:
            break;
        }
    }
}


bool SymbolTransformations::equals(const SymbolTransformations &other) const
{
    // Compare transformations
    if (size() != other.size())
        return false;
    for (int i = 0; i < size(); ++i) {
        if (at(i).type != other.at(i).type ||
            at(i).arrayIndex != other.at(i).arrayIndex ||
            at(i).member != other.at(i).member)
            return false;
    }

    return true;
}


QString SymbolTransformations::antlrTokenToStr(
        const pANTLR3_COMMON_TOKEN tok) const
{
    // Use the AST cached version, if available
    if (_typeEval && _typeEval->ast())
        return _typeEval->ast()->antlrTokenToStr(tok);
    // Otherwise use the non-caching version
    else {
        pANTLR3_STRING s = tok->getText(tok);
        return QString::fromAscii((const char*)s->chars, s->len);
    }
}


uint AssignedNode::hashPostExprSuffixes() const
{
	return hashPostExprSuffixes(postExprSuffixes, sym->ast());
}


uint AssignedNode::hashPostExprSuffixes(const ASTNodeList *pesl,
										const AbstractSyntaxTree *ast)
{
	if (!pesl)
		return 0;

	uint hash = 0;
	int rot = 0;
	QString id;
	// Hash the postfix expressions
	for (const ASTNodeList* list = pesl; list; list = list->next)
	{
		const ASTNode* node = list->item;

		switch (node->type) {
		case nt_postfix_expression_arrow:
		case nt_postfix_expression_dot:
			id = ast->antlrTokenToStr(
						node->u.postfix_expression_suffix.identifier);
			hash ^= rotl32(qHash(id), rot);
			rot = (rot + 3) % 32;
			break;
		case nt_postfix_expression_parens:
			// ignore these
			continue;
		default:
			break;
		}

		hash ^= rotl32(node->type, rot);
		rot = (rot + 3) % 32;
	}

	return hash;
}


ASTSymbol::ASTSymbol()
	: _type(stNull), _astNode(0)
{
}


ASTSymbol::ASTSymbol(const AbstractSyntaxTree* ast, const QString &name,
					 ASTSymbolType type, ASTNode *astNode)
	: _name(name), _type(type), _astNode(astNode), _ast(ast)
{
}


bool ASTSymbol::isLocal() const
{
	// built-in types lack an ASTNode and are considered to be global
	return _astNode && _astNode->scope->parent() != 0;
}



QString ASTSymbol::typeToString() const
{
	return typeToString(_type);
}


QString ASTSymbol::typeToString(ASTSymbolType type)
{
	switch (type) {
	case stNull:
		return "null";
	case stTypedef:
		return "typedef";
	case stVariableDecl:
		return "variable (declaration)";
	case stVariableDef:
		return "variable (definition)";
	case stStructOrUnionDecl:
		return "struct/union (declaration)";
	case stStructOrUnionDef:
		return "struct/union (definition)";
	case stStructMember:
		return "struct/union member";
	case stFunctionDecl:
		return "function (declaration)";
	case stFunctionDef:
		return "function (definition)";
	case stFunctionParam:
		return "parameter";
	case stEnumDecl:
		return "enum (declaration)";
	case stEnumDef:
		return "enum (definition)";
	case stEnumerator:
		return "enum value";
	}
	return "null";
}



