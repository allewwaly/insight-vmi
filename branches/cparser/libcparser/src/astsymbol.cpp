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
    // Try to simplify transformations
    if (!isEmpty()) {
        // Merge address operators and dereferences
        if ((st.type == ttDereference && last().type == ttAddress) ||
            (st.type == ttAddress && last().type == ttDereference))
        {
            // Both operations cancel each other out
            pop_back();
            return;
        }
        // Through inter-links it can happen that we count two function calls
        // in a row, so discard one
        if (st.type == ttFuncCall && last().type == ttFuncCall)
            return;
    }

    // No simplification, so append the transformation
    QList<SymbolTransformation>::append(st);
}


void SymbolTransformations::append(SymbolTransformationType type, const ASTNode *node)
{
    SymbolTransformations::append(SymbolTransformation(type, node));
}


void SymbolTransformations::append(const QString &member, const ASTNode *node)
{
    SymbolTransformations::append(SymbolTransformation(member, node));
}


void SymbolTransformations::append(int arrayIndex, const ASTNode *node)
{
    SymbolTransformations::append(SymbolTransformation(arrayIndex, node));
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
            SymbolTransformations::append(SymbolTransformation(ttDereference, p));
            // no break

        case nt_postfix_expression_dot:
            SymbolTransformations::append(
                        SymbolTransformation(
                            antlrTokenToStr(
                                p->u.postfix_expression_suffix.identifier), p));
            break;

        case nt_postfix_expression_brackets: {
            // We expect array indices to be constant
            const ASTNode* e = p->u.postfix_expression_suffix.expression ?
                        p->u.postfix_expression_suffix.expression->item : 0;
            bool ok = false;
            int index = _typeEval ?
                        _typeEval->evaluateIntExpression(e, &ok) : -1;
            SymbolTransformations::append(
                        SymbolTransformation(ok ? index : -1, p));
            break;
        }

        case nt_postfix_expression_parens:
            SymbolTransformations::append(SymbolTransformation(ttFuncCall, p));
            break;

        default:
            break;
        }
    }
}


void SymbolTransformations::append(const SymbolTransformations &other)
{
    for (int i = 0; i < other.size(); ++i)
        SymbolTransformations::append(other[i]);
}


int SymbolTransformations::derefCount() const
{
    int deref = 0;

    // Count all (de-)references before the first member access from right to
    // left
    for (int i = size() - 1; i >= 0; --i) {
        switch (at(i).type) {
        case ttArray:
        case ttDereference:
            ++deref;
            break;

        case ttAddress:
            --deref;
            break;

        default:
            goto for_exit;
        }
    }

    for_exit:
    return deref;
}


int SymbolTransformations::memberCount() const
{
    int mbrCnt = 0;

    for (int i = 0; i < size(); ++i)
        if (at(i).type == ttMember)
            ++mbrCnt;

    return mbrCnt;
}

QString SymbolTransformations::lastMember() const
{
    for (int i = size() - 1; i >= 0; --i)
        if (at(i).type == ttMember)
            return at(i).member;
    return QString();
}


bool SymbolTransformations::isPrefixOf(const SymbolTransformations &other) const
{
    // If this is a prefix of other, it must not be larger
    if (size() > other.size())
        return false;
    // Compare all transformations
    for (int i = 0; i < size(); ++i)
        if (at(i) != other.at(i))
            return false;
    return true;
}


SymbolTransformations SymbolTransformations::right(int len) const
{
    SymbolTransformations result(*this);
    if (len >= 0 && len < size()) {
        result.erase(result.begin(), result.begin() + (size() - len));
    }
    return result;
}


SymbolTransformations SymbolTransformations::left(int len) const
{
    SymbolTransformations result(*this);
    if (len >= 0 && len < size()) {
        result.erase(result.begin() + len, result.end());
    }
    return result;
}


QString SymbolTransformations::toString(const QString &symbol) const
{
    QString s(symbol);

    for (int i = 0; i < size(); ++i) {
        switch (at(i).type) {
        case ttMember:
            s += "." + at(i).member;
            break;
        case ttFuncCall:
            s += "()";
            break;
        case ttArray:
            s += QString("[%1]").arg(at(i).arrayIndex);
            break;
        case ttDereference:
            // Use array operator for easier readability
            if (i + 1 < size() && at(i + 1).type == ttMember)
                s += "->" + at(++i).member;
            else
                s = QString("(*%1)").arg(s);
            break;
        case ttAddress:
            s = QString("(&%1)").arg(s);
            break;
        }
    }

    return s;
}


uint SymbolTransformations::hash() const
{
	uint hash = 0;
	int rot = 0;
	// Hash the postfix expressions
	for (int i = 0; i < size(); ++i) {
		switch (at(i).type) {
		case ttArray:
			hash ^= rotl32(qHash(at(i).arrayIndex), rot);
			rot = (rot + 3) % 32;
			break;
		case ttMember:
			hash ^= rotl32(qHash(at(i).member), rot);
			rot = (rot + 3) % 32;
			break;
		default:
			break;
		}

		hash ^= rotl32(at(i).type, rot);
		rot = (rot + 3) % 32;
	}

	return hash;
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




