/*
 * asttypeevaluator.cpp
 *
 *  Created on: 18.04.2011
 *      Author: chrschn
 */

#include <astnode.h>
#include <asttypeevaluator.h>
#include <abstractsyntaxtree.h>
#include <astdotgraph.h>
#include <typeevaluatorexception.h>
#include <expressionevalexception.h>
#include <astscopemanager.h>
#include <realtypes.h>
#include <debug.h>
#include <bugreport.h>
#include <QStringList>
#include <unistd.h>
#include <astsourceprinter.h>
#include <bitop.h>
#include <QProcess>

#define checkNodeType(node, expected_type) \
    if ((node)->type != (expected_type)) { \
            typeEvaluatorError( \
                QString("Expected node type \"%1\", given type is \"%2\" at %3:%4") \
                    .arg(ast_node_type_to_str2(expected_type)) \
                    .arg(ast_node_type_to_str(node)) \
                    .arg(_ast->fileName()) \
                    .arg(node->start->line)); \
    }

QString ASTType::toString() const
{
    QString ret;
    for (const ASTType* type = this; type; type = type->next()) {
        if (!ret.isEmpty()) ret += "->";
        ret += realTypeToStr(type->type());
        if (!(type->type() & NumericTypes) && !type->identifier().isEmpty())
            ret += "(" + type->identifier() + ")";
    }
    return ret;
}


bool ASTType::equalTo(const ASTType* other, bool exactMatch) const
{
    static const int ptrMask = rtPointer|rtArray;
    static const int intTypes = IntegerTypes & ~rtEnum;

    const ASTType *a = this, *b = other;
    bool isPointer = false;
    while (a && b) {
        // Consider rtPointer and rtArray to be equal
        if (a->type() != b->type() && ((a->type()|b->type()) != ptrMask)) {
            // For non-exact machtes, consider all integer types to be equal
            // unless we're having a pointer to an integer type
            if (exactMatch || isPointer ||
                    (!((a->type() & intTypes) && (b->type() & intTypes))))
                return false;
        }
        // Compare the type name for non-integer types
        if (((a->type() | b->type()) & ~intTypes) &&
            (a->identifier() != b->identifier()))
            return false;
        isPointer = isPointer || ((a->type()|b->type()) & ptrMask);
        a = a->next();
        b = b->next();
    }

    return !a && !b;
}


ASTTypeEvaluator::ASTTypeEvaluator(AbstractSyntaxTree* ast, int sizeofLong,
                                   int sizeofPointer)
    : ASTWalker(ast), _sizeofLong(sizeofLong), _sizeofPointer(sizeofPointer),
      _phase(epFindSymbols), _pointsToRound(0), _assignmentsTotal(0),
      _pointsToDeadEndHits(0)
{
}


ASTTypeEvaluator::~ASTTypeEvaluator()
{
    for (ASTTypeList::iterator it = _allTypes.begin();
            it != _allTypes.end(); ++it)
        delete *it;
    _allTypes.clear();
}


bool ASTTypeEvaluator::evaluateTypes()
{
	_typeNodeStack.clear();
	_pointsToRound = 0;
	_pointsToDeadEndHits = 0;

	// Phase 1: find symbols in AST
	_phase = epFindSymbols;
	walkTree();
	if (_stopWalking)
		return false;

	// Repeat phase 2 and 3 until no more assignments were added
	do {
		_assignments = 0;
		_pointsToDeadEnds.clear();
		++_pointsToRound;
		// Phase 2: points-to analysis
		_phase = epPointsTo;
		walkTree();
		_assignmentsTotal += _assignments;

		if (!_stopWalking && _assignments > 0) {
			// Phase 3: reverse points-to analysis
			_phase = epPointsToRev;
			walkTree();
		}
#ifdef DEBUG_POINTS_TO
		if (_pointsToRound == 1)
			std::cout << std::endl;
		debugmsg("********** Round " << _pointsToRound << ": " << _assignments
				 << " assignments, " << _assignmentsTotal << " total **********");
#endif
	} while (!_stopWalking && _assignments > 0 && !interrupted());

	if (_stopWalking)
		return false;

	// Phase 4: used-as analysis
	_phase = epUsedAs;
	walkTree();
	
    if (_stopWalking)
		return false;

	// Phase 5: magic number analysis
	_phase = epMagicNumbers;
	walkTree();
	return !_stopWalking;
}


bool ASTTypeEvaluator::hasValidType(const ASTNode *node) const
{
    if (!node || !_types.value(node))
            return false;
    // Everything looks okay
    return true;
}


ASTType* ASTTypeEvaluator::copyASTType(const ASTType* src)
{
    ASTType* t = new ASTType(*src);
    _allTypes.append(t);
    return t;
}


ASTType* ASTTypeEvaluator::createASTType(RealType type, ASTType* next)
{
    ASTType* t = new ASTType(type, next);
    _allTypes.append(t);
    return t;
}


ASTType* ASTTypeEvaluator::createASTType(RealType type, const ASTNode *node,
        const QString& identifier)
{
    ASTType* t = createASTType(type, node);
    t->setIdentifier(identifier);
    return t;
}


ASTType* ASTTypeEvaluator::createASTType(RealType type, const ASTNode *node, ASTType* next)
{
    ASTType* t = createASTType(type, next);
    t->setNode(node);
    return t;
}


ASTType* ASTTypeEvaluator::copyDeepAppend(const ASTType* src, ASTType* next)
{
	if (!src)
		return next;

	ASTType *ret = 0, *cur, *prev = 0;

	while (src) {
		// Copy current element
		cur = copyASTType(src);
		// Append current element to previous one
		if (prev)
			prev->setNext(cur);
		prev = cur;
		src = src->next();
		// First node will be returned
		if (!ret)
			ret = cur;
	}

	cur->setNext(next);
	return ret;
}


ASTType* ASTTypeEvaluator::copyDeep(const ASTType* src)
{
	return copyDeepAppend(src, 0);
}


RealType ASTTypeEvaluator::resolveBaseType(const ASTType* type) const
{
    // For now just return the type itself
    if (!type)
        return rtUndefined;
    return type->type();
}


int ASTTypeEvaluator::sizeofType(RealType type) const
{
    switch (type) {
    case rtUndefined:
    	return -1;
    case rtInt8:
    case rtUInt8:
    case rtBool8:
    	return 1;
    case rtInt16:
    case rtUInt16:
    case rtBool16:
    	return 2;
    case rtInt32:
    case rtUInt32:
    case rtBool32:
    	return 4;
    case rtInt64:
    case rtUInt64:
    case rtBool64:
    	return 8;
    case rtFloat:
    	return 4;
    case rtDouble:
    	return 8;
    case rtPointer:
        return sizeofPointer();
    case rtArray:
    	return 0;
    case rtEnum:
    	return 4;
    case rtStruct:
    case rtUnion:
    case rtConst:
    case rtVolatile:
    case rtTypedef:
    	return 0;
    case rtFuncPointer:
        return sizeofPointer();
    case rtFunction:
        return 0;
    case rtVoid:
    	return 0;
    case rtVaList:
    	return -1;
//    case rtFuncCall:
//    	return 0;
    };

    return -1;
}


bool ASTTypeEvaluator::typeIsLargerThen(RealType typeA, RealType typeB) const
{
    return sizeofType(typeA) > sizeofType(typeB);
}


ASTType* ASTTypeEvaluator::typeofNode(const ASTNode *node)
{
    if (!node)
        return 0;

#   ifdef DEBUG_NODE_EVAL
    QString indent = QString("%1").arg("", _nodeStack.count() << 1);
    debugmsg(indent << "+++ Evaluating node " << ast_node_type_to_str(node)
            << " (" << QString::number((quint64)node, 16) << ")"
            << " at " << node->start->line << ":"
            << node->start->charPosition);
#   endif

    if (hasValidType(node)) {
#       ifdef DEBUG_NODE_EVAL
        debugmsg(indent << "+++ Node " << ast_node_type_to_str(node)
                << " (" << QString::number((quint64)node, 16) << ")"
                << " at " << node->start->line << ":"
                << node->start->charPosition
                << " has type " << _types[node]->toString());
#       endif
        return _types[node];
    }

    StackAutoPopper<typeof(_typeNodeStack)> autoPopper(&_typeNodeStack, node);

    // Check for loops in recursive evaluation
    for (int i = 0; i < _typeNodeStack.size() - 1; ++i) {
        if (_typeNodeStack[i] == node) {
            QString msg = QString("Detected loop in recursive type evaluation:\n"
                                  "File: %1\n")
                            .arg(_ast->fileName());
    		int cnt = 0;
            for (int j = _typeNodeStack.size() - 1; j >= 0; --j) {
                const ASTNode* n = _typeNodeStack[j];
                msg += QString("%0%1. 0x%2 %3 at line %4:%5\n")
                        .arg(cnt == 0 || j == i ? "->" : "  ")
                        .arg(cnt, 4)
                        .arg((quint64)n, 0, 16)
                        .arg(ast_node_type_to_str(n), -35)
                        .arg(n->start->line)
                        .arg(n->start->charPosition);
                ++cnt;
    		}

    		typeEvaluatorError(msg);
    	}
    }

    switch (node->type) {
    case nt_abstract_declarator:
        if (node->u.abstract_declarator.pointer)
            _types[node] = copyDeepAppend(
                    typeofNode(node->u.abstract_declarator.pointer),
                    typeofNode(node->u.abstract_declarator.direct_abstract_declarator));
        else
            _types[node] = typeofNode(node->u.abstract_declarator.direct_abstract_declarator);
        break;

    case nt_additive_expression:
        _types[node] = typeofAdditiveExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right),
                antlrTokenToStr(node->u.binary_expression.op));
        break;

    case nt_and_expression:
        _types[node] = typeofIntegerExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right),
                antlrTokenToStr(node->u.binary_expression.op));
        break;

    case nt_assembler_parameter:
        _types[node] = typeofNode(node->u.assembler_parameter.assignment_expression);
        break;

    case nt_assignment_expression:
        if (node->u.assignment_expression.lvalue)
            _types[node] = typeofNode(node->u.assignment_expression.lvalue);
        else if (node->u.assignment_expression.designated_initializer_list)
            _types[node] = typeofDesignatedInitializerList(
                        node->u.assignment_expression.designated_initializer_list);
        else
            _types[node] = typeofNode(node->u.assignment_expression.conditional_expression);

        if (!_types[node]) {
//            genDotGraphForNode(node);
            debugerr("Stopping walk");
            _stopWalking = true;
        }
        break;

    case nt_builtin_function_alignof:
    case nt_builtin_function_choose_expr:
    case nt_builtin_function_constant_p:
    case nt_builtin_function_expect:
    case nt_builtin_function_extract_return_addr:
    case nt_builtin_function_object_size:
    case nt_builtin_function_offsetof:
    case nt_builtin_function_prefetch:
    case nt_builtin_function_return_address:
    case nt_builtin_function_sizeof:
    case nt_builtin_function_types_compatible_p:
    case nt_builtin_function_va_arg:
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_end:
    case nt_builtin_function_va_start:
        _types[node] = typeofBuiltinFunction(node);
        break;

    case nt_cast_expression:
        if (node->u.cast_expression.type_name)
            _types[node] = typeofNode(node->u.cast_expression.type_name);
        else
            _types[node] = typeofNode(node->u.cast_expression.unary_expression);

        if (!_types[node]) {
//            genDotGraphForNode(node);
            debugerr("Stopping walk");
            _stopWalking = true;
        }
        break;

    case nt_compound_braces_statement:
        _types[node] = typeofCompoundBracesStatement(node);
        break;

    case nt_conditional_expression:
        if (node->u.conditional_expression.conditional_expression)
            _types[node] = typeofNode(node->u.conditional_expression.conditional_expression);
        else
            _types[node] = typeofNode(node->u.conditional_expression.logical_or_expression);
        break;

    case nt_constant_char:
        _types[node] = createASTType(rtInt8, node);
        break;

    case nt_constant_expression:
        _types[node] = typeofNode(node->u.constant_expression.conditional_expression);
        break;

    case nt_constant_float:
        _types[node] = createASTType(realTypeOfConstFloat(node), node);
        break;

    case nt_constant_int:
        _types[node] = createASTType(realTypeOfConstInt(node), node);
        break;

    case nt_constant_string:
        _types[node] = createASTType(rtArray, node, createASTType(rtInt8, node));
        _types[node]->setArraySize(
                    stringLength(node->u.constant.string_token_list));
        break;

    case nt_declaration_specifier:
        _types[node] = typeofDeclarationSpecifier(node);
        break;

    case nt_declarator:
        if (node->u.declarator.direct_declarator)
            _types[node] = typeofNode(node->u.declarator.direct_declarator);
        else
            _types[node] = typeofNode(node->u.declarator.pointer);
        break;

    case nt_direct_declarator:
        _types[node] = typeofDirectDeclarator(node);
        break;

    case nt_enumerator:
        _types[node] = typeofEnumerator(node);
        break;

    case nt_enum_specifier:
        _types[node] = typeofEnumSpecifier(node);
        break;

    case nt_equality_expression:
        _types[node] = typeofBooleanExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right));
        break;

    case nt_exclusive_or_expression:
        _types[node] = typeofIntegerExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right),
                antlrTokenToStr(node->u.binary_expression.op));
        break;

    case nt_expression_statement:
        if (node->u.expression_statement.expression) {
            // Use item at end of the list
            ASTNodeList *l = node->u.expression_statement.expression;
            while (l->next)
                l = l->next;
            _types[node] = typeofNode(l->item);
        }
        else
            _types[node] = createASTType(rtUndefined);
        break;

    case nt_external_declaration:
        if (node->u.external_declaration.assembler_statement)
            _types[node] = typeofNode(node->u.external_declaration.assembler_statement);
        else if (node->u.external_declaration.function_definition)
            _types[node] = typeofNode(node->u.external_declaration.function_definition);
        else
            _types[node] = typeofNode(node->u.external_declaration.declaration);
        break;

    case nt_function_definition:
        _types[node] = typeofNode(node->u.function_definition.declarator);
        break;

    case nt_inclusive_or_expression:
        _types[node] = typeofIntegerExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right),
                antlrTokenToStr(node->u.binary_expression.op));
        break;

    case nt_init_declarator:
        _types[node] = typeofNode(node->u.init_declarator.declarator);
        break;

    case nt_initializer:
		_types[node] = typeofInitializer(node);
        break;

    case nt_jump_statement_return:
        if (node->u.jump_statement.initializer)
            _types[node] = typeofNode(node->u.jump_statement.initializer);
        else
            _types[node] = createASTType(rtUndefined);
        break;

    case nt_logical_and_expression:
    case nt_logical_or_expression:
        _types[node] = typeofBooleanExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right));
        break;

    case nt_lvalue:
        // Could be an lvalue cast
        if (node->u.lvalue.type_name)
            _types[node] = typeofNode(node->u.lvalue.type_name);
        else
            _types[node] = typeofNode(node->u.lvalue.unary_expression);
        break;

    case nt_multiplicative_expression:
        _types[node] = typeofNumericExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right),
                antlrTokenToStr(node->u.binary_expression.op));
        break;

    case nt_parameter_declaration:
        _types[node] = typeofParameterDeclaration(node);
        break;

    case nt_pointer:
        if (node->u.pointer.pointer)
            _types[node] = createASTType(rtPointer, node, typeofNode(node->u.pointer.pointer));
        else
            _types[node] = createASTType(rtPointer, node);
        break;

    case nt_postfix_expression:
        _types[node] = typeofPostfixExpression(node);
        break;

    case nt_postfix_expression_arrow:
    case nt_postfix_expression_brackets:
    case nt_postfix_expression_dec:
    case nt_postfix_expression_dot:
    case nt_postfix_expression_inc:
    case nt_postfix_expression_parens:
        _types[node] = typeofPostfixExpressionSuffix(node);
        break;

    case nt_primary_expression:
        _types[node] = typeofPrimaryExpression(node);
        break;

    case nt_relational_expression:
        _types[node] = typeofBooleanExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right));
        break;

    case nt_shift_expression:
        _types[node] = typeofIntegerExpression(
                typeofNode(node->u.binary_expression.left),
                typeofNode(node->u.binary_expression.right),
                antlrTokenToStr(node->u.binary_expression.op));
        break;

    case nt_struct_declaration:
        _types[node] = typeofSpecifierQualifierList(
                node->u.struct_declaration.specifier_qualifier_list);
        break;

    case nt_struct_declarator:
        _types[node] = typeofNode(node->u.struct_declarator.declarator);
        break;

    case nt_struct_or_union_specifier:
        _types[node] = typeofStructOrUnionSpecifier(node);
        break;

    case nt_type_id:
        _types[node] = typeofTypeId(node);
        break;

    case nt_type_name:
        _types[node] = typeofTypeName(node);
        break;

    case nt_typeof_specification:
        if (node->u.typeof_specification.assignment_expression)
            _types[node] = typeofNode(node->u.typeof_specification.assignment_expression);
        else
            _types[node] = typeofNode(node->u.typeof_specification.parameter_declaration);
        break;

    case nt_type_specifier:
        if (node->u.type_specifier.typeof_specification)
            _types[node] = typeofNode(node->u.type_specifier.typeof_specification);
        else if (node->u.type_specifier.struct_or_union_specifier)
            _types[node] = typeofNode(node->u.type_specifier.struct_or_union_specifier);
        else if (node->u.type_specifier.enum_specifier)
            _types[node] = typeofNode(node->u.type_specifier.enum_specifier);
        else if (node->u.type_specifier.builtin_type_list)
            _types[node] = typeofBuiltinType(node->u.type_specifier.builtin_type_list, node);
        else if (node->u.type_specifier.type_id)
            _types[node] = typeofNode(node->u.type_specifier.type_id);
        break;

    case nt_unary_expression_builtin:
        _types[node] = typeofBuiltinFunction(node->u.unary_expression.builtin_function);
        break;

    case nt_unary_expression_dec:
    case nt_unary_expression_inc:
        _types[node] = typeofNode(node->u.unary_expression.unary_expression);
        break;


    case nt_unary_expression:
        _types[node] = typeofNode(node->u.unary_expression.postfix_expression);
        break;

    case nt_unary_expression_op:
        _types[node] = typeofUnaryExpressionOp(node);
        break;

    default:
        debugerr("Unhandled node type: " << ast_node_type_to_str(node)
                << " (" << node->type << ")");
        break;
    }

    if (!_types.value(node)) {
//        genDotGraphForNode(node);
//      debugerr(_ast->fileName() << ":" << node->start->line << ": "
//              << "Failed to resolve type for node "
//              << ast_node_type_to_str(node));
        typeEvaluatorError(QString("Failed to resolve type for node %1 at %2:%3:%4")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line)
                .arg(node->start->charPosition));
    }

#   ifdef DEBUG_NODE_EVAL
    debugmsg(indent << "+++ Node " << ast_node_type_to_str(node)
            << " (" << QString::number((quint64)node, 16) << ")"
            << " at " << node->start->line << ":"
            << node->start->charPosition
            << " has type " << _types[node]->toString());
#   endif

    return _types[node];
}


ASTType* ASTTypeEvaluator::typeofIntegerExpression(ASTType* lt, ASTType* rt,
		const QString& op) const
{
    if (!rt)
        return lt;
    RealType lres = resolveBaseType(lt);
    RealType rres = resolveBaseType(rt);

    // Is this integer arithmetics?
    if ((lres & IntegerTypes) && (rres & IntegerTypes)) {
        if (typeIsLargerThen(lres, rres))
            return lt;
        else
            return rt;
    }
    // Is it an (illegal) void operation?
    else if (lres == rtVoid)
        return lt;
    else if (rres == rtVoid)
        return rt;
    else
        typeEvaluatorError(
                QString("Cannot determine resulting type of \"%1 %2 %3\" "
                		"at lines %4 and %5")
                    .arg(realTypeToStr(lres))
                    .arg(op)
                    .arg(realTypeToStr(rres))
                    .arg(lt->node() ? lt->node()->start->line : 0)
                    .arg(rt->node() ? rt->node()->start->line : 0));

    return 0;
}


ASTType* ASTTypeEvaluator::typeofNumericExpression(ASTType* lt, ASTType* rt,
		const QString& op) const
{
    if (!rt)
        return lt;
    RealType lres = resolveBaseType(lt);
    RealType rres = resolveBaseType(rt);

    // Is this floating point arithmetics?
    if ((lres & FloatingTypes) && (rres & FloatingTypes))
        return typeIsLargerThen(lres, rres) ? lt : rt;
    if ((lres & IntegerTypes) && (rres & FloatingTypes))
        return rt;
    else if ((rres & IntegerTypes) && (lres & FloatingTypes))
        return lt;
    else
        return typeofIntegerExpression(lt, rt, op);
}


ASTType* ASTTypeEvaluator::typeofAdditiveExpression(ASTType* lt, ASTType* rt,
		const QString& op)
{
    if (!rt)
        return lt;
    int lres = resolveBaseType(lt);
    int rres = resolveBaseType(rt);

    // Is this pointer arithmetics?
    if ((lres & (rtArray|rtPointer)) && (rres & IntegerTypes))
        return lt;
    else if ((rres  & (rtArray|rtPointer)) && (lres & IntegerTypes))
        return rt;
    // Pointers/Arrays can be substracted, leading to a long value
    else if ((lres & (rtArray|rtPointer)) &&
             ((rres & (rtArray|rtPointer)) &&
             op == "-"))
    	return createASTType(realTypeOfLong(), lt->node());
    else
        return typeofNumericExpression(lt, rt, op);
}


ASTType* ASTTypeEvaluator::typeofBooleanExpression(ASTType* lt, ASTType* rt)
{
    return rt ? createASTType(rtInt32) : lt;
}


ASTType* ASTTypeEvaluator::typeofDesignatedInitializerList(const ASTNodeList *list)
{
    if (!list)
        return 0;

    ASTType* ret = 0;

    // Find enclosing initializer node
    const ASTNode* initializer = list->item->parent;
    while (initializer) {
        if (initializer->type == nt_initializer &&
            !initializer->u.initializer.assignment_expression)
            break;
        initializer = initializer->parent;
    }

    if (initializer)
        ret = typeofNode(initializer);
    else {
        typeEvaluatorError(
                    QString("Could not find enclosing initializer at %3:%4:%5")
                    .arg(_ast->fileName())
                    .arg(list->item->start->line)
                    .arg(list->item->start->charPosition));

    }

    // Go through complete list and find type of designated initializers
    for (; list; list = list->next) {
        const ASTNode* node = list->item;
        checkNodeType(node, nt_designated_initializer);

        // Return cached value if existent
        if (hasValidType(node))
            ret = typeofNode(node);
        else {

#ifdef DEBUG_NODE_EVAL
            debugmsg("+++ Node " << ast_node_type_to_str(node)
                     << " (" << QString::number((quint64)node, 16) << ")"
                     << " at " << node->start->line << ":"
                     << node->start->charPosition);
#endif

            // Array or struct initializer?
            if (node->u.designated_initializer.identifier) {
                QString id = antlrTokenToStr(node->u.designated_initializer.identifier);

                // If we have an ASTType node, try to find most inner struct/union
                while (ret && !(ret->type() & StructOrUnion))
                    ret = ret->next();
                const ASTSymbol* sym = ret ?
                            ret->node()->childrenScope->find(id) : 0;

                if (!sym || sym->type() != stStructMember) {
//                    _ast->printScopeRek(ret ? ret->node()->childrenScope : node->scope);
                    typeEvaluatorError(
                                QString("Could not find member \"%1\" in %2 at %3:%4:%5")
                                .arg(id)
                                .arg(ret ? ret->toString() : QString("(n/a)"))
                                .arg(_ast->fileName())
                                .arg(node->start->line)
                                .arg(node->start->charPosition));
                }
                // Use the symbol's type
                ret = typeofSymbol(sym);
            }
            // Must be an array initializer
            else {
                // Make sure the type fits
                if (!(ret->type() & (rtPointer|rtArray))) {
                    typeEvaluatorError(
                                QString("Type of designated initializer is \"%1\", expected "
                                        "a Pointer or Array at %2:%3")
                                .arg(ret->toString())
                                .arg(_ast->fileName())
                                .arg(initializer->start->line));
                }
                // Skip outer pointer/array
                ret = ret->next();
            }

            _types[node] = ret;
        }
    }

    return ret;
}

/**
 * Returns the return type of the inner-most function \a node is contained in.
 * @param node the node contained in a function
 * @return
 */
ASTType* ASTTypeEvaluator::embeddingFuncReturnType(const ASTNode *node)
{
    while (node && node->type != nt_function_definition)
        node = node->parent;

    assert(node != 0);
    ASTType* ret = typeofNode(node);
    assert(ret != 0);
    assert(ret->type() == rtFuncPointer);

    return ret->next();
}

/**
 * Returns the symbol of the inner-most function \a node is contained in.
 * @param node the node contained in a function
 * @return
 */
const ASTSymbol* ASTTypeEvaluator::embeddingFuncSymbol(const ASTNode *node)
{
    // Did we evaluate this symbol before?
    if (_symbolOfNode.contains(node))
        return _symbolOfNode[node];

    while (node && node->type != nt_function_definition)
        node = node->parent;

    assert(node != 0);
    const ASTNode* dd = node
            ->u.function_definition.declarator
            ->u.declarator.direct_declarator;
    QString name = antlrTokenToStr(dd->u.direct_declarator.identifier);

    const ASTSymbol* sym = node->scope->find(name, ASTScope::ssSymbols);
    assert(sym && sym->type() == stFunctionDef);

    if (sym)
        _symbolOfNode.insert(node, sym);
    return sym;
}

/**
 * Returns the type that an array or struct initializer "{...}" expects at a
 * the position of the given initializer node \a node.
 *
 * If we have:
 * \code
 * struct foo {
 *     int i;
 *     struct bar {
 *         float r;
 *         char* c;
 *     }
 * };
 *
 * struct foo f = { x, { y, z } };
 * \endcode
 * then this function will return Int32 for the initializer node at position
 * \c x, Float for the initializer at postion \c y, and Pointer->Int8 for \c z,
 * regardless of the actual types of x, y, and z.
 *
 * @param node initializer nodes at positions such as shown for \c x, \c y,
 * and \c z.
 * @return
 */
ASTType* ASTTypeEvaluator::expectedTypeAtInitializerPosition(const ASTNode *node)
{
    checkNodeType(node, nt_initializer);
    checkNodeType(node->parent, nt_initializer);

    // Get the parent's type
    ASTType *pt = typeofNode(node->parent);

    // For an array, the initializer's type equals the array's base type
    switch (pt->type()) {
    case  rtArray:
    case  rtPointer:
        return pt->next();
    // Find type of member for structs and unions
    case rtStruct:
    case rtUnion: {
        const ASTNode* strSpec = pt->node();
        checkNodeType(strSpec, nt_struct_or_union_specifier);
        if (!strSpec->u.struct_or_union_specifier.isDefinition)
            typeEvaluatorError(
                    QString("Resolved struct type \"%1\" is not a "
                            "definition at %2:%3")
                        .arg(antlrTokenToStr(
                                strSpec
                                ->u.struct_or_union_specifier.identifier))
                        .arg(_ast->fileName())
                        .arg(node->start->line));
        // Count the position of this node in the parent's initializer list
        int pos = 1;
        for (const ASTNodeList* list =
                node->parent->u.initializer.initializer_list;
            list && list->item != node;
            list = list->next)
        {
            ++pos;
        }
        const int orig_pos = pos;
        // Get the element of the struct definition at position pos
        for (const ASTNodeList* list =
                strSpec->u.struct_or_union_specifier.struct_declaration_list;
            list && pos;
            list = list->next)
        {
            const ASTNode* strDec = list->item;
            // Does the struct_declaration have at least one
            // struct_declarator?
            if (strDec->u.struct_declaration.struct_declarator_list) {
                // Go through the whole list
                for (const ASTNodeList* list2 =
                        strDec->u.struct_declaration.struct_declarator_list;
                    list2 && pos;
                    list2 = list2->next)
                {
                    if (--pos == 0)
                        return typeofNode(list2->item);
                }
            }
            // No declarators, type is only the specifier_qualifier_list
            else {
                if (--pos == 0)
                    return typeofSpecifierQualifierList(
                            strDec->u.struct_declaration.specifier_qualifier_list);
            }
        }
        // We should not get here!
        typeEvaluatorError(
                QString("Failed to resolve the %1th position of "
                        " struct \"%2\" at %3:%4")
                    .arg(orig_pos)
                    .arg(antlrTokenToStr(
                            strSpec
                            ->u.struct_or_union_specifier.identifier))
                    .arg(_ast->fileName())
                    .arg(node->start->line));
        break;
    }
    default:
        typeEvaluatorError(
                QString("Unexpected type \"%1\" for an initializer at %2:%3")
                    .arg(pt->toString())
                    .arg(_ast->fileName())
                    .arg(node->start->line));
        break;
    }

    return 0;
}


ASTType* ASTTypeEvaluator::typeofInitializer(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_initializer);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    if (node->u.initializer.assignment_expression)
        _types[node] = typeofNode(node->u.initializer.assignment_expression);
    else {

    	switch (node->parent->type) {
    	// Top-most initializer, i.e. "struct foo f = { ... }"
    	case nt_init_declarator:
		// Nested initializer with member name, i.e. ".foo = { ... }"
    	case nt_assignment_expression:
		// Designated initializer, i.e. "f = (struct foo) { ... }"
    	case nt_cast_expression:
			_types[node] = typeofNode(node->parent);
			break;
		// Nested initializer without member name, i.e. "{ ... }"
		case nt_initializer: {
		    _types[node] = expectedTypeAtInitializerPosition(node);
			break;
		}
    	default:
    		typeEvaluatorError(
    				QString("Unexpected parent type \"%1\" for node \"%2\" at %3:%4")
    					.arg(ast_node_type_to_str(node->parent))
    					.arg(ast_node_type_to_str(node))
    					.arg(_ast->fileName())
    					.arg(node->start->line));
    		break;
    	}
    }

    return _types[node];
}


ASTType* ASTTypeEvaluator::typeofParameterDeclaration(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_parameter_declaration);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);


    ASTType* ret = typeofNode(node->u.parameter_declaration.declaration_specifier);
    for (const ASTNodeList* list = node->u.parameter_declaration.declarator_list;
    	 list;
    	 list = list->next)
    {
        const ASTNode* d_ad = list->item;
    	ret = preprendPointersArrays(d_ad, ret);
    }

    return ret;
}


ASTType* ASTTypeEvaluator::typeofSymbolDeclaration(const ASTSymbol* sym)
{
    if (!sym || !sym->astNode())
        return 0;
    checkNodeType(sym->astNode(), nt_declaration);

    ASTType* ret = typeofNode(sym->astNode()->u.declaration.declaration_specifier);
    ret = preprendPointersArraysOfIdentifier(sym->name(), sym->astNode(), ret);

    return ret;
}


ASTType* ASTTypeEvaluator::typeofSymbolFunctionDef(const ASTSymbol* sym)
{
    if (!sym || !sym->astNode())
        return 0;
    checkNodeType(sym->astNode(), nt_function_definition);

    const ASTNode* declarator = sym->astNode()->u.function_definition.declarator;
    return typeofNode(declarator->u.declarator.direct_declarator);
}


ASTType* ASTTypeEvaluator::typeofSymbolFunctionParam(const ASTSymbol* sym)
{
    if (!sym || !sym->astNode())
        return 0;
    checkNodeType(sym->astNode(), nt_parameter_declaration);

    ASTType* ret = typeofNode(sym->astNode()->u.parameter_declaration.declaration_specifier);

    // Find identifier matching sym to see if it's a pointer
    for (const ASTNodeList* list = sym->astNode()
            ->u.parameter_declaration.declarator_list;
         list;
         list = list->next)
    {
        ret = preprendPointersArrays(list->item, ret);
    }

    return ret;
}


ASTType* ASTTypeEvaluator::typeofEnumerator(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_enumerator);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    // Per default all an enumerator equals a regular integer
    return _types[node] = createASTType(rtInt32, node);
}


ASTType* ASTTypeEvaluator::typeofStructDeclarator(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_struct_declarator);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    const ASTNode* sd = node->parent;

    _types[node] = preprendPointersArrays(
    		node->u.struct_declarator.declarator, typeofNode(sd));

    return _types[node];
}


ASTType* ASTTypeEvaluator::typeofStructOrUnionSpecifier(const ASTNode* node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_struct_or_union_specifier);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    const ASTNode* s_u = node->u.struct_or_union_specifier.struct_or_union;
    QString id = antlrTokenToStr(node->u.struct_or_union_specifier.identifier);
    RealType rt = (s_u->type == nt_struct_or_union_struct) ? rtStruct : rtUnion;

    // If this is just a struct declaration, try to find its definition
    if (!node->u.struct_or_union_specifier.isDefinition) {
        // A struct declaration should always contain an identifier
        if (id.isEmpty())
            typeEvaluatorError(
                    QString("Struct declaration lacks an identifier at %1:%2")
                            .arg(_ast->fileName())
                            .arg(node->start->line));
        // We have to search in upward scope because id may be a struct
        // declaration in the current scope
        for (ASTScope *scope = node->scope; scope; scope = scope->parent()) {
            const ASTSymbol* def = scope->find(id, ASTScope::ssCompoundTypes);
            if (def && def->type() == stStructOrUnionDef)
                return _types[node] = typeofNode(def->astNode());
        }
    }

    // Otherwise create a new ASTType node for a struct/union definition or
    // forward declaration
    _types[node] = createASTType(rt, node);
    if (!id.isEmpty())
        _types[node]->setIdentifier(id);

    return _types[node];
}


ASTType* ASTTypeEvaluator::typeofEnumSpecifier(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_enum_specifier);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    QString id = antlrTokenToStr(node->u.enum_specifier.identifier);

    return _types[node] = createASTType(rtEnum, node, id);
}


ASTType* ASTTypeEvaluator::typeofSpecifierQualifierList(const ASTNodeList *sql)
{
    if (!sql)
        return 0;

    ASTType *ret = 0;
    int count = 0;

    for (; sql; sql = sql->next) {
        const ASTNode* node = sql->item;
    	// We're not interested in type_qualifier nodes
    	if (!node || node->type == nt_type_qualifier)
    		continue;

    	if (!typeofNode(node))
    	    typeEvaluatorError(QString("Failed to resolve type of %1 at %2:%3")
    	            .arg(ast_node_type_to_str(node))
    	            .arg(_ast->fileName())
    	            .arg(node->start->line));

    	switch (++count) {
    	// If we only have one type_specifier, we can use it as is
    	case 1:
    		ret = typeofNode(node);
    		break;
		// If this is the 2nd type_specifier, we have to make a copy first
    	case 2:
    		ret = copyDeep(ret);
    		// No break
    	default:
    		ret = copyDeepAppend(typeofNode(node), ret);
    		break;
    	}
    }

    return ret;
}


ASTType* ASTTypeEvaluator::typeofTypeName(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_type_name);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    ASTType* ret = typeofSpecifierQualifierList(node->u.type_name.specifier_qualifier_list);
    ret = preprendPointersArrays(node->u.type_name.abstract_declarator, ret);

    return _types[node] = ret;
}


ASTType* ASTTypeEvaluator::typeofUnaryExpressionOp(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_unary_expression_op);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    QString op = antlrTokenToStr(node->u.unary_expression.unary_operator);
    // Operators returning integers, so assume resulting type is type of cast
    // expression
    if (op == "+" || op == "-" || op == "~")
    	_types[node] = typeofNode(node->u.unary_expression.cast_expression);
    // Boolean expression
    else if (op == "!")
    	_types[node] = createASTType(rtInt32, node);
    // Pointer dereferencing
    else if (op == "*") {
    	ASTType* ret = typeofNode(node->u.unary_expression.cast_expression);
    	// Is top type a pointer type?
    	if (!ret || !(ret->type() & (rtPointer|rtArray|rtFuncPointer)))
            typeEvaluatorError(
                    QString("We expected a pointer or array here at %1:%2")
                            .arg(_ast->fileName())
                            .arg(node->start->line));
    	// If this expression dereferences  a function pointer, don't remove
    	// the first rtFuncPointer type, because this is done by the
    	// postfix_expression_parens node later on as part of the invocation
    	if (ret->type() == rtFuncPointer &&
			(node->parent->parent->type != nt_unary_expression_op ||
			 "*" != antlrTokenToStr(
					 node->parent->parent->u.unary_expression.unary_operator)))
    	{
    	    bool hasParens = false;
    	    // Find enclosing postfix_expression node
            const ASTNode* postEx = node->parent;
    	    while (postEx && postEx->type != nt_postfix_expression)
    	        postEx = postEx->parent;

    	    if (postEx) {
                // Check if we really have a postfix_expression_parens node
                for (const ASTNodeList* list = !postEx ? 0 :
                        postEx->u.postfix_expression.postfix_expression_suffix_list;
                    list;
                    list = list->next)
                {
                    if (list->item->type == nt_postfix_expression_parens) {
                        hasParens = true;
                        break;
                    }
                }
    	    }
    	    // Remove top-most FuncPointer type if and only if enclosing
    	    // postfix_expression has no postfix_expression_parens suffix
			_types[node] = hasParens ? ret : ret->next();
    	}
    	else
    		_types[node] = ret->next();
    }
    // Pointer obtaining
    else if (op == "&") {
    	ASTType* ret = typeofNode(node->u.unary_expression.cast_expression);
        // Obtaining address of a function does not result in another pointer
        // level
        if (ret && ret->isFunction() && !ret->ampersandSkipped())
            ret->setAmpersandSkipped(true);
        else
            ret = createASTType(rtPointer, node, ret);
        _types[node] = ret;
    }
    // Label as value, see http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
    else if (op == "&&") {
    	_types[node] = createASTType(rtPointer, node, createASTType(rtVoid, node));
    }
    else {
    	typeEvaluatorError(QString("Unknown operator: %1").arg(op));
    	return 0;
    }

    return _types[node];
}


ASTType* ASTTypeEvaluator::typeofCompoundBracesStatement(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_compound_braces_statement);

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

	ASTType* type = 0;

    // In case we have a statement list, the last statement gives the type
    if (node->u.compound_braces_statement.declaration_or_statement_list) {
        // Last statement constitutes the type
        const ASTNodeList* list = node->u.compound_braces_statement.declaration_or_statement_list;
    	while (list->next)
			 list = list->next;
    	// If last statement is no expression_statement, then type is "void"
    	if (list->item && list->item->type == nt_expression_statement)
    	    type = typeofNode(list->item);
    	else
    	    type = createASTType(rtVoid, node);
    }

	if (!type || type->type() == rtUndefined) {
		typeEvaluatorError(
		        QString("Could not resolve type of compound braces statement "
		                "at %1:%2")
		                .arg(_ast->fileName())
		                .arg(node->start->line));
	}

	return _types[node] = type;
}


ASTType* ASTTypeEvaluator::typeofBuiltinFunction(const ASTNode *node)
{
    if (!node)
        return 0;
    // Compare to first and last built-in type in enum ASTNodeType
    if (node->type < nt_builtin_function_alignof ||
            node->type > nt_builtin_function_va_start)
        typeEvaluatorError(
            QString("Unexpected node type \"%1\" at %3:%4")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line));

    // Return cached value if existent
    if (hasValidType(node))
    	return typeofNode(node);

    // References:
    // * http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
    // * http://gcc.gnu.org/onlinedocs/gcc/Return-Address.html
    // * http://gcc.gnu.org/onlinedocs/gcc/Object-Size-Checking.html
    // * man stdarg.h

    switch (node->type) {
    case nt_builtin_function_alignof:
    case nt_builtin_function_object_size:
    case nt_builtin_function_offsetof:
    case nt_builtin_function_sizeof:
    	_types[node] = createASTType(realTypeOfULong(), node);
    	break;

    case nt_builtin_function_choose_expr:
        // We actually would need to evaluate the constant_expression in order
        // to tell if the resulting type is assignment_expression1 or
        // assignment_expression2, so this let's hope for the best...
        _types[node] = typeofNode(node->u.builtin_function_choose_expr.assignment_expression1);
        break;

    case nt_builtin_function_constant_p:
    case nt_builtin_function_types_compatible_p:
    	_types[node] = createASTType(rtInt32, node);
    	break;

    case nt_builtin_function_expect:
		_types[node] = createASTType(realTypeOfLong(), node);
		break;

    case nt_builtin_function_extract_return_addr:
    case nt_builtin_function_return_address:
		_types[node] = createASTType(rtPointer, node,
							createASTType(rtVoid, node));
		break;

    case nt_builtin_function_prefetch:
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_end:
    case nt_builtin_function_va_start:
		_types[node] = createASTType(rtVoid, node);
		break;

    case nt_builtin_function_va_arg:
		_types[node] = typeofNode(node->u.builtin_function_va_xxx.type_name);
		break;

    default:
        typeEvaluatorError(
                QString("Unknown built-in function type \"%1\" at %2:%3")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line));
    	return 0;
    }

    return _types[node];
}



ASTType* ASTTypeEvaluator::typeofSymbol(const ASTSymbol* sym)
{
    if (!sym)
        return 0;

    switch (sym->type()) {
    case stNull:
        return 0;

    case stTypedef:
        return typeofSymbolDeclaration(sym);

    case stVariableDecl:
    case stVariableDef:
    case stStructOrUnionDecl:
    case stStructOrUnionDef:
        return typeofNode(sym->astNode());

    case stStructMember:
        return typeofNode(sym->astNode());

    case stFunctionDecl:
        return typeofNode(sym->astNode());

    case stFunctionDef:
        return typeofNode(sym->astNode());

    case stFunctionParam:
        return typeofNode(sym->astNode());

    case stEnumDecl:
    case stEnumDef:
        return typeofNode(sym->astNode());


    case stEnumerator:
        return typeofEnumerator(sym->astNode());
        break;
    }

    return 0;
}


ASTType* ASTTypeEvaluator::typeofBuiltinType(const pASTTokenList list,
											 const ASTNode *node)
{
	QString tokens;
	RealType type = evaluateBuiltinType(list, &tokens);
	if (type == rtUndefined)
		return 0;

	ASTType* astType = createASTType(type, node);
	astType->setIdentifier(tokens);

	return astType;
}


const ASTSymbol* ASTTypeEvaluator::findSymbolOfDirectDeclarator(
        const ASTNode *node, bool enableExcpetions)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_direct_declarator);

    const ASTSymbol* sym = 0;

    // Did we evaluate this symbol before?
    if (_symbolOfNode.contains(node))
        sym = _symbolOfNode[node];
    else {
        if (!node->u.direct_declarator.identifier)
            typeEvaluatorError(
                        QString("Direct declarator has no identifier at %1:%2:%3")
                        .arg(_ast->fileName())
                        .arg(node->start->line)
                        .arg(node->start->charPosition));

        QString id = antlrTokenToStr(node->u.direct_declarator.identifier);
        if ( (sym = node->scope->find(id)) )
            _symbolOfNode.insert(node, sym);
    }

    if (!sym && enableExcpetions) {
//        _ast->printScopeRek(node->scope);
        typeEvaluatorError(
                    QString("Could not find symbol \"%1\" at %2:%3:4")
                    .arg(antlrTokenToStr(node->u.direct_declarator.identifier))
                    .arg(_ast->fileName())
                    .arg(node->start->line)
                    .arg(node->start->charPosition));
    }

    return sym;
}


const ASTSymbol* ASTTypeEvaluator::findSymbolOfPrimaryExpression(
        const ASTNode *node, bool enableExcpetions)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_primary_expression);

    const ASTSymbol* sym = 0;
    ASTType* t = 0;

    // Did we evaluate this symbol before?
    if (_symbolOfNode.contains(node))
        sym = _symbolOfNode[node];
    else {
        if (!node->u.primary_expression.identifier)
            typeEvaluatorError(
                        QString("Primary expression has no identifier at %1:%2:%3")
                        .arg(_ast->fileName())
                        .arg(node->start->line)
                        .arg(node->start->charPosition));

        QString id = antlrTokenToStr(node->u.primary_expression.identifier);

        // Are we a direct child of a __builtin_offsetof node?
        const ASTNode* offsetofNode = node->parent;
        bool found = false;
        while (offsetofNode && !found) {
            switch (offsetofNode->type) {
            case nt_builtin_function_offsetof:
                found = true;
                break;
                // We can stop searching for the following node types:
            case nt_assembler_parameter:
            case nt_assembler_statement:
            case nt_builtin_function_alignof:
            case nt_builtin_function_constant_p:
            case nt_builtin_function_expect:
            case nt_builtin_function_extract_return_addr:
            case nt_builtin_function_object_size:
            case nt_builtin_function_prefetch:
            case nt_builtin_function_return_address:
            case nt_builtin_function_sizeof:
            case nt_builtin_function_types_compatible_p:
            case nt_builtin_function_va_arg:
            case nt_builtin_function_va_copy:
            case nt_builtin_function_va_end:
            case nt_builtin_function_va_start:
            case nt_compound_braces_statement:
            case nt_compound_statement:
            case nt_declarator_suffix_brackets:
            case nt_designated_initializer:
            case nt_expression_statement:
            case nt_function_definition:
            case nt_iteration_statement_do:
            case nt_iteration_statement_for:
            case nt_iteration_statement_while:
            case nt_jump_statement_goto:
            case nt_jump_statement_return:
            case nt_labeled_statement:
            case nt_labeled_statement_case:
            case nt_labeled_statement_default:
            case nt_postfix_expression_brackets:
            case nt_postfix_expression_parens:
            case nt_selection_statement_if:
            case nt_selection_statement_switch:
            case nt_typeof_specification:
                offsetofNode = 0; // exit loop
                break;
                // A binary expression with a right-hand side is not valid
                // inside __builtin_offsetof
            case nt_equality_expression:
            case nt_logical_and_expression:
            case nt_logical_or_expression:
            case nt_relational_expression:
                if (offsetofNode->u.binary_expression.right) {
                    offsetofNode = 0;
                    break;
                }
                // no break
            default:
                offsetofNode = offsetofNode->parent;
                break;
            }
        }

        // If this is a child of a __builtin_offsetof, then find
        // struct/union definition of type
        if (offsetofNode) {
            t = typeofNode(offsetofNode->u.builtin_function_offsetof.type_name);
            // If we have an ASTType node, try to find most inner struct/union
            while (t && !(t->type() & StructOrUnion))
                t = t->next();
            if (t && (sym = t->node()->childrenScope->find(id)))
                _symbolOfNode.insert(node, sym);
        }
        // Otherwise do a normal resolution of symbol in scope of node
        else {
            sym = node->scope->find(id);
            // Make sure we did not find a struct member within a sizeof(),
            // typeof() or other builtin expressions or functions
            if (sym && sym->type() == stStructMember) {
                const ASTNode* n = node->parent;
                bool typeExpected = false;
                while (n) {
                    switch (n->type) {
                    case nt_builtin_function_alignof:
                    case nt_builtin_function_object_size:
                    case nt_builtin_function_sizeof:
                    case nt_builtin_function_types_compatible_p:
                    case nt_typeof_specification:
                        typeExpected = true;
                        n = 0;
                        break;
                    default:
                        n = n->parent;
                    }
                }

                // If we are in some builtin expression that expects a type or
                // variable, search again for symbol in upper scope
                if (typeExpected) {
                    ASTScope* scope = node->scope->parent();
                    const ASTSymbol* nonMbrSym = sym;
                    while (scope && nonMbrSym &&
                           nonMbrSym->type() == stStructMember)
                    {
                        nonMbrSym = scope->find(id);
                        scope = scope->parent();
                    }
                    // Did we find a different symbol?
                    if (nonMbrSym &&  nonMbrSym->type() != stStructMember)
                        sym = nonMbrSym;
                    // Otherwise, just return the member symbol and see what
                    // happens...
                }
            }
        }
    }

    if (!sym && enableExcpetions) {
//        _ast->printScopeRek(t ? t->node()->childrenScope : node->scope);
        typeEvaluatorError(
                QString("Could not find symbol \"%1\" at %2:%3:%4")
                    .arg(antlrTokenToStr(node->u.primary_expression.identifier))
                    .arg(_ast->fileName())
                    .arg(node->start->line)
                    .arg(node->start->charPosition));
    }

    return sym;
}


ASTType* ASTTypeEvaluator::typeofPrimaryExpression(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node,  nt_primary_expression);

    // Return cached value if existent
    if (hasValidType(node))
        return typeofNode(node);

    if (node->u.primary_expression.constant)
        _types[node] = typeofNode(node->u.primary_expression.constant);
    else if (node->u.primary_expression.expression)
        // Use first expression in list
        _types[node] = typeofNode(node->u.primary_expression.expression->item);
    else if (node->u.primary_expression.compound_braces_statement)
        _types[node] = typeofNode(node->u.primary_expression.compound_braces_statement);
    else if (node->u.primary_expression.identifier) {
        const ASTSymbol* sym = findSymbolOfPrimaryExpression(node);
        _types[node] = typeofSymbol(sym);
    }

    return _types[node];
}


ASTType* ASTTypeEvaluator::typeofDirectDeclarator(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_direct_declarator);

    // Return cached value if existent
    if (hasValidType(node))
        return typeofNode(node);

    const ASTNode* declarator = node->parent;
    assert(declarator != 0);
    checkNodeType(declarator, nt_declarator);

    ASTType* type = 0;
    bool doPrepend = true;

    // A declarator may only occur in the following five definitions
    switch (declarator->parent->type) {
    case nt_direct_declarator:
        // Must be a function pointer like "void (*foo)()"
    	assert(declarator->parent->parent->type == nt_declarator ||
    			declarator->parent->parent->type == nt_abstract_declarator);

    	// Start with the declared basic type
    	if (declarator->parent->parent->parent->parent->type == nt_declaration)
    		type = typeofNode(declarator->parent->parent->parent->parent->u.declaration.declaration_specifier);
    	else if (declarator->parent->parent->parent->type == nt_parameter_declaration)
    		type = typeofNode(declarator->parent->parent->parent->u.parameter_declaration.declaration_specifier);
    	else if (declarator->parent->parent->parent->parent->type == nt_struct_declaration)
    		type = typeofSpecifierQualifierList(declarator->parent->parent->parent->parent->u.struct_declaration.specifier_qualifier_list);
    	else
    		typeEvaluatorError(
    				QString("Unexpected parent chain for node %1,\n"
    						"    %2->type == %3\n"
    						"    %4->type == %5\n"
    						"    %6->type == %7\n"
    						"    %8->type == %9\n")
    						.arg(ast_node_type_to_str(node))
    						.arg("declarator->parent", -46)
    						.arg(ast_node_type_to_str(declarator->parent))
    						.arg("declarator->parent->parent", -46)
    						.arg(ast_node_type_to_str(declarator->parent->parent))
    						.arg("declarator->parent->parent->parent", -46)
    						.arg(ast_node_type_to_str(declarator->parent->parent->parent))
    						.arg("declarator->parent->parent->parent->parent", -46)
    						.arg(ast_node_type_to_str(declarator->parent->parent->parent->parent)));

        // Prepend any pointer of the basic type
        type = preprendPointersArrays(declarator->parent->parent, type);
        break;

    case nt_function_definition:
    	if (declarator->parent->u.function_definition.declaration_specifier)
    		type = typeofNode(declarator->parent->u.function_definition.declaration_specifier);
    	else
    		// Default return type of a function is an integer
    		type = createASTType(rtInt32, node);
    	break;

    case nt_init_declarator:
        // If this is a top-level direct_declarator of a function definition,
        // use the type of the inner direct_declarator as the type
        if (node->u.direct_declarator.declarator) {
            type = typeofNode(node->u.direct_declarator.declarator);
            doPrepend = false;
        }
        // Otherwise evaluate the type as usual
        else
            type = typeofNode(declarator->parent->parent->u.declaration.declaration_specifier);
    	break;

    case nt_struct_declarator:
    	type = typeofStructDeclarator(declarator->parent);
    	doPrepend = false;
    	break;

    case nt_parameter_declaration:
    	type = typeofParameterDeclaration(declarator->parent);
    	doPrepend = false;
    	break;

    default:
    	typeEvaluatorError(
    			QString("Found a %1 as a child of %2 at %3:%4")
    				.arg(ast_node_type_to_str(declarator))
    				.arg(ast_node_type_to_str(declarator->parent))
    				.arg(_ast->fileName())
    				.arg(declarator->start->line));
    	break;
    }

    if (doPrepend)
    	type = preprendPointersArrays(declarator, type);

//    QStringList sl;
//    debugmsg(QString("Type of symbol \"%1\" in scope 0x%2 is %3 at %4:%5")
//    			.arg(antlrTokenToStr(node->u.direct_declarator.identifier))
//    			.arg((quint64)node->scope, 0, 16)
//    			.arg(type->toString())
//    			.arg(_ast->fileName())
//    			.arg(node->start->line));

    return _types[node] = type;
}


ASTType* ASTTypeEvaluator::typeofDeclarationSpecifier(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_declaration_specifier);

    // Return cached value if existent
    if (hasValidType(node))
        return typeofNode(node);

    ASTType* type = 0;
    for (const ASTNodeList* list =
            node->u.declaration_specifier.declaration_specifier_list;
        list;
        list = list->next)
    {
        if (list->item->type != nt_type_specifier)
            continue;
        // We expect to find exactly one type_secifier node, no more, no less
        if (type)
            typeEvaluatorError(
                    QString("Found more than one type_specifier at %2:%3")
                        .arg(_ast->fileName())
                        .arg(node->start->line));
        type = typeofNode(list->item);
    }

    if (!type)
        typeEvaluatorError(
                QString("Did not find any type_specifier at %2:%3")
                    .arg(_ast->fileName())
                    .arg(node->start->line));

    return type;
}


ASTType* ASTTypeEvaluator::typeofPostfixExpression(const ASTNode *node)
{
    if (!node)
        return 0;
    checkNodeType(node, nt_postfix_expression);

    // Return cached value if existent
    if (hasValidType(node))
        return typeofNode(node);

    // If this postfix expression has suffixes, return the type of the last one
    if (node->u.postfix_expression.postfix_expression_suffix_list) {
        const ASTNodeList* list =
                node->u.postfix_expression.postfix_expression_suffix_list;
        while (list->next)
            list = list->next;
        return _types[node] = typeofNode(list->item);
    }
    // Otherwise return type of primary expression
    return _types[node] =
            typeofNode(node->u.postfix_expression.primary_expression);
}


ASTType* ASTTypeEvaluator::typeofPostfixExpressionSuffix(const ASTNode *node)
{
    if (!node)
        return 0;
    // Check node type
    switch(node->type) {
    case nt_postfix_expression_arrow:
    case nt_postfix_expression_brackets:
    case nt_postfix_expression_dec:
    case nt_postfix_expression_dot:
    case nt_postfix_expression_inc:
    case nt_postfix_expression_parens:
        break;
    default:
        typeEvaluatorError(
            QString("Expected node type \"%1\", given type is \"%2\" at %3:%4")
                .arg("postfix_expression_suffix")
                .arg(ast_node_type_to_str(node))
                .arg(_ast->fileName())
                .arg(node->start->line));
        // no break
    }

    // Return cached value if existent
    if (hasValidType(node))
        return typeofNode(node);

    const ASTNode* pe = node->parent;
    assert(pe->type == nt_postfix_expression);
    ASTType* type = typeofNode(pe->u.postfix_expression.primary_expression);
    // Scope from which to search for types and identifiers
    ASTScope* startScope = 0;

    for (const ASTNodeList* list =
            pe->u.postfix_expression.postfix_expression_suffix_list;
         list;
         list = list->next)
    {
        const ASTNode* pes = list->item;
        // If not yet set, start with the scope of the postfix expression
        if (!startScope)
        	startScope = pes->scope;

        switch (pes->type) {

        case nt_postfix_expression_brackets:
            // Array operator, i.e., dereferencing
            if (!type || !(type->type() & (rtFuncPointer|rtPointer|rtArray)))
                typeEvaluatorError(
                        QString("Expected a pointer or array here at %1:%2")
                                .arg(_ast->fileName())
                                .arg(pes->start->line));
            // Remove top pointer/array type
            type = type->next();
            break;

        case nt_postfix_expression_dec:
        case nt_postfix_expression_inc:
            // type remains the same
            break;

        case nt_postfix_expression_arrow:
        case nt_postfix_expression_dot: {
//        case nt_postfix_expression_star: {
            // find embedding struct or union
            QString memberName = antlrTokenToStr(pes->u.postfix_expression_suffix.identifier);
            ASTType* t = type;
            while (t && !(t->type() & StructOrUnion))
                t = t->next();
            if (!t) {
            	// Find identifier of primary expression
            	QString primExprId = antlrTokenToStr(
            			pe->u.postfix_expression.primary_expression
            				->u.primary_expression.identifier);
                typeEvaluatorError(
                        QString("Could not find embedding struct or union "
                        		"\"%1\" for member \"%2\" in type %3 at %4:%5:%6")
							.arg(primExprId)
                            .arg(memberName)
                            .arg(type ? type->toString() : QString("NULL"))
                            .arg(_ast->fileName())
                            .arg(pes->start->line)
                            .arg(pes->start->charPosition));
            }

            // Queue for search of members in nested structs
            QList<const ASTNode*> queue;

            // Get symbol of struct/union definition which embeds memberName
            ASTSymbol *structDeclSym = 0, *memberSym = 0;
            // If the ASTNode has no identifier, it must be an anonymous struct
            // definition, either in a direct declaration or in a typedef. In
            // that case, we have to search in the scope of the struct
            // definition rather then in the scope of the postfix expression.
            assert(! (t->type() & NumericTypes));
            if (t->identifier().isEmpty()) {
                if (t->node())
                    queue.push_back(t->node());
                else
                    typeEvaluatorError(
                            QString("No AST node for embedding struct or union for member \"%1\" at %2:%3")
                                .arg(memberName)
                                .arg(_ast->fileName())
                                .arg(pes->start->line));
            }
            // Otherwise search for struct's identifier in scope of postfix
            // expression
            else {
                for (ASTScope* scope = startScope;
                     scope; scope = scope->parent())
                {
                    // Search in tagged types first
                    structDeclSym = scope->find(t->identifier(),
                            ASTScope::ssCompoundTypes);
					// If not found, widen search to symbols
					if (!structDeclSym)
                        structDeclSym = scope->find(t->identifier(),
                                ASTScope::ssSymbols|ASTScope::ssTypedefs);

                    // If symbol is still null, it cannot be resolved
                    if (!structDeclSym ||
                            structDeclSym->type() == stStructOrUnionDef)
                        break;
                }

                if (!structDeclSym) {
//                    _ast->printScopeRek(startScope);
                    typeEvaluatorError(
                            QString("Could not resolve type \"%1\" of member "
                                    "\"%2\" in %3 at %4:%5:%6")
                                .arg(t->identifier())
                                .arg(memberName)
                                .arg(type->toString())
                                .arg(_ast->fileName())
                                .arg(pes->start->line)
                                .arg(pes->start->charPosition));
                }
                else
                    queue.push_back(structDeclSym->astNode());
            }

            // Recursively search for identifier in the members of current
            // struct and any nesting anonymous structs or unions
            // See http://gcc.gnu.org/onlinedocs/gcc/Unnamed-Fields.html
            while (!memberSym && !queue.isEmpty()) {
                const ASTNode* inner = queue.front();
				queue.pop_front();

				// Sanity checks
				if (!inner) continue;
				checkNodeType(inner, nt_struct_or_union_specifier);

				// Search in struct/union's scope for member
				memberSym = inner->childrenScope->find(memberName,
						ASTScope::ssSymbols|ASTScope::ssCompoundTypes);
				// Make sure we found a struct member
				if (memberSym && memberSym->type() != stStructMember)
					memberSym = 0;

				// Search for possible other structs/unions inside of
				// current struct/union
				for (const ASTNodeList* list =
						inner->u.struct_or_union_specifier.struct_declaration_list;
					list && !memberSym;
					list = list->next)
				{
					const ASTNode* strDec = list->item;
					// Search for struct/union specifications in
					// specifier_qualifier_list
					for (const ASTNodeList* sql =
							strDec->u.struct_declaration.specifier_qualifier_list;
						sql;
						sql = sql->next)
					{
						const ASTNode* ts_tq = sql->item;
						// Is this a struct/union definition?
						if (ts_tq->type == nt_type_specifier &&
								ts_tq->u.type_specifier.struct_or_union_specifier)
						{
							const ASTNode* su = ts_tq->u.type_specifier.struct_or_union_specifier;
							// Limit to anonymous struct/union definitions
							if (su->u.struct_or_union_specifier.isDefinition &&
								!su->u.struct_or_union_specifier.identifier)
								queue.push_back(su);
						}
					}
				}
            }

			// We should have resolved the member by now
			if (!memberSym) {
                typeEvaluatorError(
                        QString("Could not resolve member \"%1.%2\" at %3:%4:%5")
                            .arg(structDeclSym->name())
                            .arg(memberName)
                            .arg(_ast->fileName())
                            .arg(pes->start->line)
                            .arg(pes->start->charPosition));
            }

			// Set new starting scope to scope of member found
			startScope = memberSym->astNode()->childrenScope;

            type = typeofSymbol(memberSym);
            break;
        }

        case nt_postfix_expression_parens: {
            // Function operator, i.e., function call
            if (!type || !(type->type() & (rtFuncPointer)))
                typeEvaluatorError(
                        QString("Expected a function pointer type here instead of \"%1\" at %2:%3:%4")
                                .arg(type ? type->toString() : QString())
                                .arg(_ast->fileName())
                                .arg(pes->start->line)
                                .arg(pes->start->charPosition));
            // Remove top function pointer type
            type = type->next();
            break;
        }

        default:
            typeEvaluatorError(
                    QString("Not a legal postfix expression suffix: %1 at %2:%3")
                        .arg(ast_node_type_to_str(pes))
                        .arg(_ast->fileName())
                        .arg(pes->start->line));
            break;
        }

        // Stop at given node
        if (pes == node)
            break;
    }

    return _types[node] = type;
}


RealType ASTTypeEvaluator::realTypeOfConstFloat(const ASTNode* node,
                                                double *value) const
{
    if (!node)
        return rtUndefined;
    checkNodeType(node, nt_constant_float);

    RealType type = rtUndefined;
    QString s = antlrTokenToStr(node->u.constant.literal);


    // Is this a float value?
    if (s.endsWith('f', Qt::CaseInsensitive)) {
        type = rtFloat;
        s = s.left(s.size() - 1);
    }
    // Default type is double
    else {
        if (s.endsWith('d', Qt::CaseInsensitive))
            s = s.left(s.size() - 1);
        type = rtDouble;
    }

    if (value) {
        bool ok;
        *value = s.toDouble(&ok);
        // Check conversion result
        if (!ok)
            typeEvaluatorError(QString("Failed to convert constant \"%1\" to "
                                       "float at %2:%3:%4")
                          .arg(s)
                          .arg(_ast->fileName())
                          .arg(node->start->line)
                          .arg(node->start->charPosition));
    }

    return type;
}


RealType ASTTypeEvaluator::realTypeOfConstInt(const ASTNode* node,
                                              quint64* value) const
{
    if (!node)
        return rtUndefined;
    checkNodeType(node, nt_constant_int);

    RealType type = rtUndefined;
    QString s = antlrTokenToStr(node->u.constant.literal);
    QString suffix;

    // Remove any size or signedness specifiers from the string
    while (s.endsWith('l', Qt::CaseInsensitive) ||
           s.endsWith('u', Qt::CaseInsensitive))
    {
        suffix.prepend(s.right(1));
        s = s.left(s.size() - 1);
    }
    // Use the C convention to choose the correct base
    bool ok;
    quint64 val = s.toULongLong(&ok, 0);
    if (value)
        *value = val;
    if (!ok)
        typeEvaluatorError(QString("Failed to parse constant integer \"%1\" "
                                   "at %2:%3:%4")
                           .arg(s)
                           .arg(_ast->fileName())
                           .arg(node->start->line)
                           .arg(node->start->charPosition));
    // Without suffix given, find the smallest type fitting the value
    if (suffix.isEmpty()) {
        if (val < (1ULL << 31))
            type = rtInt32;
        else if (val < (1ULL << 32))
            type = rtUInt32;
        else if (val < (1ULL << 63))
            type = rtInt64;
        else
            type = rtUInt64;
    }
    // Check explicit size and constant suffixes
    else {
        suffix = suffix.toLower();
        // Extend the size to 64 bit, depending on the number of l's and
        // the architecture of the guest
        if (suffix.startsWith("ll") || suffix.endsWith("ll") ||
                ((suffix.startsWith('l') || suffix.endsWith('l')) &&
                 sizeofLong() > 4))
            type = rtInt64;
        else
            type = rtInt32;
        // Honor the unsigned flag. If not present, the constant is signed.
        if (suffix.startsWith('u') || suffix.endsWith('u'))
            type = (type == rtInt32) ? rtUInt32 : rtUInt64;
    }

    return type;
}


const ASTNode *ASTTypeEvaluator::findIdentifierInIDL(const QString& identifier,
        const ASTNodeList *initDeclaratorList)
{
    // Find identifier in list
    for (const ASTNodeList* list = initDeclaratorList;
         list;
         list = list->next)
    {
        const ASTNode* init_dec = list->item;
        if (!init_dec) continue;
        const ASTNode* dec = init_dec->u.init_declarator.declarator;
        if (!dec) continue;
        const ASTNode* ddec = dec->u.declarator.direct_declarator;
        if (!ddec) continue;
        // Is this function_declarator or a declarator_identifier?
        if (!ddec->u.direct_declarator.identifier) {
            // Must be a function_declarator
            dec = ddec->u.direct_declarator.declarator;
            if (!dec->u.declarator.direct_declarator)
                typeEvaluatorError(
                        QString("Unexpected null value for direct_declarator "
                                "searching for identifier \"%1\" at %2:%3")
                                .arg(identifier)
                                .arg(_ast->fileName())
                                .arg(init_dec->start->line));
            ddec = dec->u.declarator.direct_declarator;
        }
        QString id = antlrTokenToStr(ddec->u.direct_declarator.identifier);
        // Is this the identifier we're looking for?
        if (id == identifier)
            return ddec;
    }

    return 0;
}


ASTType* ASTTypeEvaluator::preprendPointersArraysOfIdentifier(
        const QString& identifier, const ASTNode *declaration, ASTType* type)
{
    if (!declaration)
        return 0;
    checkNodeType(declaration, nt_declaration);

    // Find identifier matching given name to append pointers, if any
    const ASTNode* ddec = findIdentifierInIDL(
    		identifier,
    		declaration->u.declaration.init_declarator_list);

    if (ddec)
        type = preprendPointersArrays(ddec->parent, type);
    else
        typeEvaluatorError(
                QString("Did not find identifier \"%1\" in "
                        "init_declarator_list at %2:%3")
                        .arg(identifier)
                        .arg(_ast->fileName())
                        .arg(declaration->start->line));

    return type;
}


ASTType* ASTTypeEvaluator::preprendPointersArrays(const ASTNode *d_ad, ASTType* type)
{
    if (!d_ad)
        return type;
    if (d_ad->type != nt_declarator && d_ad->type != nt_abstract_declarator)
        typeEvaluatorError(
            QString("Unexpected node type \"%1\" at %3:%4")
                .arg(ast_node_type_to_str(d_ad))
                .arg(_ast->fileName())
                .arg(d_ad->start->line));

    type = preprendPointers(d_ad, type);

    if (d_ad->type == nt_declarator) {
        if (d_ad->u.declarator.direct_declarator)
            type = preprendArrays(d_ad->u.declarator.direct_declarator, type);
    }
    else /*if (d_ad->type == nt_abstract_declarator)*/ {
        if (d_ad->u.abstract_declarator.direct_abstract_declarator)
            type = preprendArrays(d_ad->u.abstract_declarator.direct_abstract_declarator, type);
    }

    return type;
}


ASTType* ASTTypeEvaluator::preprendPointers(const ASTNode *d_ad, ASTType* type)
{
    if (!d_ad)
        return type;
    if (d_ad->type != nt_declarator && d_ad->type != nt_abstract_declarator)
        typeEvaluatorError(
            QString("Unexpected node type \"%1\" at %3:%4")
                .arg(ast_node_type_to_str(d_ad))
                .arg(_ast->fileName())
                .arg(d_ad->start->line));

    // Distinguish between declarator and abstract_declarator
    const ASTNode* ptr = 0;
    if (d_ad->type == nt_declarator)
        ptr = d_ad->u.declarator.pointer;
    else /*if (d_ad->type == nt_abstract_declarator)*/
        ptr = d_ad->u.abstract_declarator.pointer;

    // Distinguish between function pointer and regular pointer: The declarator
    // of a function pointer is always child of a direct_declarator node.
    RealType ptrType = rtPointer;
    bool skipFirst = false;
    if (d_ad->parent->type == nt_direct_declarator ||
		d_ad->parent->type == nt_direct_abstract_declarator)
    {
    	ptrType = rtFuncPointer;
    	// If there is at least one declarator_suffix_parens in the suffix list,
    	// then we need to skip the first pointer, because it is added when we
    	// encounter the declarator_suffix_parens anyways
        const ASTNodeList* list = d_ad->parent->type == nt_direct_declarator ?
    	        d_ad->parent->u.direct_declarator.declarator_suffix_list :
    	        d_ad->parent->u.direct_abstract_declarator.abstract_declarator_suffix_list;
    	for (; list; list = list->next) {
            const ASTNode* ds_ads = list->item;
    	    if (ds_ads->type == nt_abstract_declarator_suffix_parens ||
                ds_ads->type == nt_declarator_suffix_parens)
    	    {
    	        skipFirst = true;
    	        break;
    	    }
    	}
    }
    // If the type is FuncPointer and there was no pointer to be skipped at its
    // definition (e.g., as in "typedef void (foo)()"), than we have yet to skip
    // the first pointer
    else if (type && type->type() == rtFuncPointer && !type->pointerSkipped())
    {
        skipFirst = true;
    }

    // Add one pointer type node for every asterisk in the declaration
    while (ptr) {
    	// For function pointers, the first asterisk is ignored, because GCC
    	// treats "void (foo)()" and "void (*foo)()" equally.
    	if (skipFirst) {
    		skipFirst = false;
            // We must not change the original type information, so copy it
            // before any modification
            type = copyDeep(type);
            type->setPointerSkipped(true);
    	}
    	else {
    		type = createASTType(ptrType, ptr, type);
    		if (ptrType == rtFuncPointer)
    		    type->setPointerSkipped(true);
    	}

        ptr = ptr->u.pointer.pointer;
    }
    return type;
}


ASTType* ASTTypeEvaluator::preprendArrays(const ASTNode *dd_dad, ASTType* type)
{
    if (!dd_dad)
        return type;
    if (dd_dad->type != nt_direct_declarator &&
            dd_dad->type != nt_direct_abstract_declarator)
        typeEvaluatorError(
            QString("Unexpected node type \"%1\" at %3:%4")
                .arg(ast_node_type_to_str(dd_dad))
                .arg(_ast->fileName())
                .arg(dd_dad->start->line));

    const ASTNodeList* list = 0;
    bool isFuncPtr = false;
    if (dd_dad->type == nt_direct_abstract_declarator) {
    	type = preprendPointersArrays(
    			dd_dad->u.direct_abstract_declarator.abstract_declarator,
    			type);
        list = dd_dad->u.direct_abstract_declarator.abstract_declarator_suffix_list;
    }
    else {
        list = dd_dad->u.direct_declarator.declarator_suffix_list;
        isFuncPtr = dd_dad->u.direct_declarator.declarator &&
                dd_dad->u.direct_declarator.declarator->u.declarator.isFuncDeclarator;
    }

    // Add one array type node for every pair of brackets in the declaration
    while (list) {
        const ASTNode* ds_ads = list->item;
        if (!ds_ads)
            typeEvaluatorError(
                    QString("Emtpy declarator suffix found at %2:%3")
                        .arg(_ast->fileName())
                        .arg(dd_dad->start->line));
        // Brackets lead to an array type
        if (ds_ads->type == nt_declarator_suffix_brackets ||
                ds_ads->type == nt_abstract_declarator_suffix_brackets)
        {
            type = createASTType(rtArray, ds_ads, type);
            // Evaluate expression in brackets, if possible
            bool ok = false;
            int size = evaluateIntExpression(
                        ds_ads->u.declarator_suffix.constant_expression, &ok);
            if (ok)
                type->setArraySize(size);
        }
        // Parens lead to a function pointer type
        else if (ds_ads->type == nt_declarator_suffix_parens ||
                ds_ads->type == nt_abstract_declarator_suffix_parens)
        {
            type = createASTType(rtFuncPointer, ds_ads, type);
            type->setIsFunction(!isFuncPtr);
        }
        else
            typeEvaluatorError(
                    QString("Declarator suffix is of unexpected type %1 at %2:%3")
                        .arg(ast_node_type_to_str(ds_ads))
                        .arg(_ast->fileName())
                        .arg(ds_ads->start->line));
        list = list->next;
    }
    return type;
}


QString ASTTypeEvaluator::postfixExpressionToStr(const ASTNode* postfix_exp,
        const ASTNode* last_pes) const
{
    if (!postfix_exp)
        return QString();
    assert(postfix_exp->type == nt_postfix_expression);


    const ASTNode* prim_exp = postfix_exp->u.postfix_expression.primary_expression;
    ASTSourcePrinter printer(_ast);
    QString var = printer.toString(prim_exp).trimmed();
    for (const ASTNodeList* l =
            postfix_exp->u.postfix_expression.postfix_expression_suffix_list;
            l; l = l->next)
    {
        const ASTNode* pes = l->item;

        switch (pes->type) {
        case nt_postfix_expression_arrow:
            var += "->" + antlrTokenToStr(pes->u.postfix_expression_suffix.identifier);
            break;
        case nt_postfix_expression_brackets:
            var += "[]";
            break;
        case nt_postfix_expression_dec:
            var += "++";
            break;
        case nt_postfix_expression_dot:
            var += "." + antlrTokenToStr(pes->u.postfix_expression_suffix.identifier);
            break;
        case nt_postfix_expression_inc:
            var += "--";
            break;
        case nt_postfix_expression_parens:
            if (pes->u.postfix_expression_suffix.argument_expression_list)
                var += "(...)";
            else
                var += "()";
            break;
        default:
            debugerr("Unexpected type: " << ast_node_type_to_str(pes));
            break;
        }

        // Stop at last expression
        if (pes == last_pes)
            break;
    }

    return var;
}


void ASTTypeEvaluator::appendTransformations(
        const ASTNode *node, SymbolTransformations *transformations) const
{
    if (!node || !transformations)
        return;

    switch (node->type) {
    case nt_postfix_expression:
        transformations->append(
                    node->u.postfix_expression.postfix_expression_suffix_list);
        break;

    case nt_unary_expression_op: {
        QString op = antlrTokenToStr(node->u.unary_expression.unary_operator);

        if (op == "&" || op == "&&") {
            transformations->append(ttAddress, node);
            if (op == "&&")
                transformations->append(ttAddress, node);
        }
        else if (op == "*")
            transformations->append(ttDereference, node);
        break;
    }

    default:
        break;
    }
}


void ASTTypeEvaluator::afterChildren(const ASTNode *node, int /* flags */)
{
    if (!node)
    	return;

    if (interrupted()) {
        _stopWalking = true;
        return;
    }

    try {
        switch (node->type) {
        case nt_direct_declarator:
            // The direct declarator must have an identifier
            if (!node->u.direct_declarator.identifier)
                return;

            if (_phase == epFindSymbols)
                collectSymbols(node);
            else if (_phase == epPointsTo)
                evaluateIdentifierPointsTo(node);
            break;

        case nt_jump_statement_return:
            // We are only interested in jump statements that return a value
            if (!node->u.jump_statement.initializer)
                return;

            if (_phase == epPointsTo)
                evaluateIdentifierPointsTo(node);
            break;

        case nt_primary_expression:
            // The primary expressions must have an identifier.  If the identifier
            // has a leading dot, it is used as initializer identifier.
            if (!node->u.primary_expression.identifier)
                return;

            if (_phase == epFindSymbols)
                collectSymbols(node);
            else if (_phase == epPointsTo)
                evaluateIdentifierPointsTo(node);
            else if (_phase == epPointsToRev)
                evaluateIdentifierPointsToRev(node);
            else if (_phase == epUsedAs)
                evaluateIdentifierUsedAs(node);
            else if (_phase == epMagicNumbers)
                evaluateMagicNumbers(node);
            break;

        case nt_init_declarator:
            if (_phase == epMagicNumbers)
                evaluateMagicNumbers(node);
            break;

        default:
            break;
        }

    }
    catch (TypeEvaluatorException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = node;
        while (n && n->parent && n->type != nt_function_definition)
            n = n->parent;
        reportErr(e, n, &e.ed);
    }
    catch (ExpressionEvalException& e) {
        // Make sure we at least have the full postfix expression
        const ASTNode* n = e.node;
        for (int i = 0;
             i < 3 && n && n->parent && n->type != nt_function_definition;
             ++i)
            n = n->parent;
        reportErr(e, n, 0);
    }
}

void ASTTypeEvaluator::evaluateMagicNumbers(const ASTNode * /*node*/)
{
    // The default implementation does nothing.
    return;
}

void ASTTypeEvaluator::reportErr(const GenericException& e, const ASTNode* node,
                                 const TypeEvalDetails* ed) const
{
    // General information
    QString msg = QString("%0: %1\n\n").arg(e.className()).arg(e.message);
    // Evaluation details, if given
    if (ed && ed->sym && ed->primExNode) {
        msg += QString("Details (may be incomplete):\n%0\n\n")
                .arg(typeChangeInfo(*ed));
    }
    // Source file name
    msg += QString("File: %0").arg(_ast->fileName());
    // Source code, if node is given
    if (node) {
        ASTSourcePrinter printer(_ast);
        msg += QString("\n"
                       "------------------[Source]------------------\n"
                       "%0"
                       "------------------[/Source]-----------------")
                .arg(printer.toString(node, true));
    }
    // Write report
    BugReport::reportErr(msg, e.file, e.line);
}


void ASTTypeEvaluator::collectSymbols(const ASTNode *node)
{
    if (!node)
        return;

    const ASTSymbol* sym = 0;

    // Find symbol of source node
    switch (node->type) {
    case nt_direct_declarator:
        sym = findSymbolOfDirectDeclarator(node, false);
        break;
    case nt_primary_expression:
        sym = findSymbolOfPrimaryExpression(node, false);
        break;
    default:
        typeEvaluatorError(QString("Unexpected node type: %1")
                           .arg(ast_node_type_to_str(node)));
    }

    // No symbols are created for non-interesting identifiers such as
    // parameter declarations of function declarations or goto labels.
    // Ignore enumerator symbols, they are constant values
    if (!sym || sym->type() == stEnumerator)
        return;

    // For primary expressions, see when we have to stop to consider
    // any transformations
    const ASTNode* stopAppendNode = 0;
    if (node->type == nt_primary_expression) {
        TypeEvalDetails ed(this);
        ed.sym = sym;
        ed.srcNode = node;
        ed.primExNode = node;
        ed.rootNode = node->parent;
        ed.followInterLinks = false;
        evaluateTypeFlow(&ed);
        stopAppendNode = ed.rootNode;
    }

    const ASTNode* root = node->parent, *rNode = node;
    TransformedSymbol transSym(sym, this);
    bool doAppend = true;

    // Climb up the tree until the symbol goes out of scope
    while (root && root->scope != sym->astNode()->scope->parent()) {
        if (root == stopAppendNode)
            doAppend = false;
        // Find out the situation in which the identifier is used
        switch (root->type) {
        case nt_assignment_expression:
            // Is this in the right-hand of an assignment expression?
            if (root->u.assignment_expression.lvalue &&
               rNode == root->u.assignment_expression.assignment_expression)
            {
                _symbolsBelowNode[rNode].insertMulti(sym, transSym);
            }
            break;

        case nt_init_declarator:
            // Is this in the right-hand side of an init declarator?
            if (rNode == root->u.init_declarator.initializer &&
                root->u.init_declarator.declarator)
            {
                _symbolsBelowNode[rNode].insertMulti(sym, transSym);
            }
            break;

        case nt_jump_statement_return:
            // An identifier underneath a return should be the initializer
            assert(rNode == root->u.jump_statement.initializer);
            _symbolsBelowNode[rNode].insertMulti(sym, transSym);
            break;

        case nt_postfix_expression:
        case nt_unary_expression_op:
            if (doAppend)
                appendTransformations(root, &transSym.transformations);
            break;

        default:
            break;
        }

        // Otherwise go up
        rNode = root;
        root = root->parent;
    }
}


bool ASTTypeEvaluator::canHoldPointerValue(RealType type) const
{
    switch (type) {
    case rtFloat:
    case rtDouble:
    case rtBool8:
    case rtBool16:
    case rtBool32:
    case rtBool64:
    case rtInt8:
    case rtUInt8:
    case rtInt16:
    case rtUInt16:
    case rtEnum:
        return false;

    case rtInt32:
    case rtUInt32:
        return (sizeofPointer() <= 4);

    case rtInt64:
    case rtUInt64:
        return (sizeofPointer() <= 8);

    default:
        typeEvaluatorError(QString("Expected a numeric type here instead of %1")
                           .arg(realTypeToStr(type)));
    }

    return true;
}


void ASTTypeEvaluator::evaluateIdentifierPointsTo(const ASTNode *node)
{
    if (!node)
        return;

    PointsToEvalState es(this);
    es.srcNode = es.root = node;

    // Find symbol of source node
    switch (node->type) {
    case nt_direct_declarator:
        es.sym = findSymbolOfDirectDeclarator(node, false);
        break;
    case nt_primary_expression:
        es.sym = findSymbolOfPrimaryExpression(node, false);
        break;
    case nt_jump_statement_return:
        es.sym = embeddingFuncSymbol(node);
        break;
    default:
        typeEvaluatorError(QString("Unexpected node type: %1")
                           .arg(ast_node_type_to_str(node)));
    }

    // No symbols are created for non-interesting identifiers such as
    // parameter declarations of function declarations or goto labels
    if (!es.sym)
        return;
    // Ignore enumerator symbols, they are constant values
    if (es.sym->type() == stEnumerator)
        return;
    // Ignore symbols of integer type that cannot hold a pointer
    const ASTType* type = typeofSymbol(es.sym);
    if ((type->type() & NumericTypes) && !canHoldPointerValue(type->type()))
        return;

    es.followedSymStack.push(TransformedSymbol(es.sym, this));

    evaluateIdentifierPointsToRek(&es);
}


int ASTTypeEvaluator::evaluateIdentifierPointsToRek(PointsToEvalState *es)
{
    // Check for loop in recursion
    if (es->evalNodeStack.contains(es->root))
        return 0;

    // Push current root on the recursion tracking stack, gets auto-popped later
    StackAutoPopper<ASTNodeStack> autoPopper(&es->evalNodeStack, es->root);

#ifdef DEBUG_POINTS_TO
    QString s;
    for (ASTNodeNodeHash::const_iterator it = es->interLinks.begin();
         it != es->interLinks.end(); ++it)
    {
        s += QString("\n    %1:%2:%3 => %4:%5:%6")
                .arg(ast_node_type_to_str(it.key()))
                .arg(it.key()->start->line)
                .arg(it.key()->start->charPosition)
                .arg(ast_node_type_to_str(it.value()))
                .arg(it.value()->start->line)
                .arg(it.value()->start->charPosition);
        if (it.value()->type == nt_primary_expression) {
            s += QString(" (%1)").arg(
                        findSymbolOfPrimaryExpression(it.value())->name());
        }
    }

    debugmsg(QString("Evaluating points-to for \"%1\" at %2:%3 (links: %4)%5")
             .arg(es->sym->name())
             .arg(es->srcNode->start->line)
             .arg(es->srcNode->start->charPosition)
             .arg(es->interLinks.size())
             .arg(s));
#endif

    QString op;

    ASTNodeNodeMHash::const_iterator it, e = _assignedNodesRev.end();
    PointsToEvalState rek_es(this);
    SymbolTransformations localTrans(this);
    const ASTNode* assigned = 0;
    int assignments = 0;

#ifdef DEBUG_POINTS_TO
    QString debugPrefix = es->debugPrefix;

    debugmsg(QString("%1:%2 %3 %4 interLinks: %5")
             .arg(es->srcNode->start->line)
             .arg(es->srcNode->start->charPosition, -3)
             .arg(es->sym->name(), -10)
             .arg(es->interLinks.count(), 2)
             .arg(debugPrefix));
#endif

    while (es->root) {
        // Was this node assigned to other variables?
        it = _assignedNodesRev.find(es->root);
        if (it != e) {
            // If last link involved transformations, they must be a prefix
            // of the local transformations
            if (es->lastLinkTrans.isPrefixOf(localTrans)) {
#ifdef DEBUG_POINTS_TO
                int cnt = 0, total = _assignedNodesRev.count(es->root), w = 0;
                for (int i = total; i > 0; i /= 10)
                    ++w;
#endif

                SymbolTransformations combinedTrans = combineTansformations(
                            es->transformations, localTrans,
                            es->lastLinkTrans);

                TransformedSymbol curSym, ts;

                for (; it != e && it.key() == es->root; ++it) {
#ifdef DEBUG_POINTS_TO
                    ++cnt;
#endif
                    // For recursion, the initial link must have been added last
                    // round, older links have been considered before
                    if (es->interLinks.isEmpty() &&
                        it->addedInRound != _pointsToRound - 1)
                        continue;

                    // Do not follow any symbol twice
                    ts = TransformedSymbol(it->sym, it->transformations);
                    if (es->followedSymStack.contains(ts))
                        continue;

                    curSym = TransformedSymbol(es->sym, combinedTrans);
                    const uint symHash = qHash(curSym);
                    const uint linkTargetHash =
                            qHash(it->node) ^  qHash(it->transformations);

                    // Check if the current transformed symbol leads to a dead
                    // end when following this inter-link.
                    if (_pointsToRound > 1 &&
                        _pointsToDeadEnds.contains(symHash, linkTargetHash))
                    {
                        ++_pointsToDeadEndHits;
#ifdef DEBUG_POINTS_TO
                        debugmsg("Dead end no. " << _pointsToDeadEndHits
                                 << " for " << combinedTrans.toString(es->sym->name()));
#endif
                        continue;
                    }

                    // Recurive points-to analysis
                    rek_es = *es;
                    rek_es.followedSymStack.push(ts);
                    rek_es.root = it->node;
                    rek_es.lastLinkTrans = it->transformations;
                    rek_es.transformations = combinedTrans;
                    rek_es.interLinks.insert(es->root,  rek_es.root);
#ifdef DEBUG_POINTS_TO
                    rek_es.debugPrefix = QString("%1 %2/%3").arg(debugPrefix)
                            .arg(cnt, w).arg(total);
#endif

                    int ret = evaluateIdentifierPointsToRek(&rek_es);

                    // If we had no assignments, mark this state as a dead end.
                    // It only depends on the current, transformed symbol as
                    // well as the target node and its link transformations.
                    if (!ret && _pointsToRound > 1)
                        _pointsToDeadEnds.insert(symHash, linkTargetHash);
                }
            }
#ifdef DEBUG_POINTS_TO
            else {
                debugmsg(QString("Skipping all links from %1 %2:%3 because %4 "
                                 "is no prefix of %5")
                         .arg(ast_node_type_to_str(es->root))
                         .arg(es->root->start->line)
                         .arg(es->root->start->charPosition)
                         .arg(es->lastLinkTrans.toString(es->sym->name()))
                         .arg(localTrans.toString(es->sym->name())));
            }
#endif
        }

        // Find out the situation in which the identifier is used
        switch (es->root->type) {
        case nt_additive_expression:
        case nt_and_expression:
        case nt_equality_expression:
        case nt_exclusive_or_expression:
        case nt_inclusive_or_expression:
        case nt_logical_and_expression:
        case nt_logical_or_expression:
        case nt_multiplicative_expression:
        case nt_relational_expression:
        case nt_shift_expression:
            // A binary operation with two operands yields no valid lvalue
            if (es->root->u.binary_expression.left &&
                es->root->u.binary_expression.right)
            {
                es->validLvalue = false;
            }
            break;

        case nt_assignment_expression:
            // Is this in the left-hand of an assignment expression?
            if (es->prevNode == es->root->u.assignment_expression.lvalue &&
                (assigned = es->root->u.assignment_expression.assignment_expression))
            {
                QString op = antlrTokenToStr(
                            es->root->u.assignment_expression.assignment_operator);
                // Ignore assignment operators other than "="
                if (op != "=")
                    return assignments;

                // Check if local transformations are a prefix of last
                // link's transformations
                if (!es->lastLinkTrans.isPrefixOf(localTrans)) {
#ifdef DEBUG_POINTS_TO
                    debugmsg(QString("Skipping left-hand symbol \"%1\" "
                                     "because %2 is no prefix of %3 at "
                                     "%4:%5:%6")
                             .arg(es->sym->name())
                             .arg(es->lastLinkTrans.toString(es->sym->name()))
                             .arg(localTrans.toString(es->sym->name()))
                             .arg(_ast ? _ast->fileName() : QString("-"))
                             .arg(es->root->start->line)
                             .arg(es->root->start->charPosition));
#endif
                    return assignments;
                }

                // Ignore cases where we have a = b, replaced left-hand a with
                // right-hand b, leading to b = b.
                if (es->interLinks.contains(assigned))
                    return assignments;

                // Ignore right-hand side expressions that cannot hold a pointer
                // value
                const ASTType* type = typeofNode(assigned);
                if ((type->type() & NumericTypes) &&
                    !canHoldPointerValue(type->type()))
                    return assignments;


                // If the dereference level of the lvalue is smaller or
                // equal to the last link's dereference level, we shouldn't
                // have followed that link, e.g.:
                //
                // void *p, **q, *r;
                // *q = &p;    // q -> &p => lastLinkTrans.derefCount() = 1
                // *q = r;     // localTrans.derefCount() = 1, should be > 1
                //
                // Only if the last link's dereference count is truely lower
                // than the local, the link was followed correctly, e.g.:
                //
                // void *p, **q, *r;
                // q = &p;     // q -> &p => lastLinkTrans.derefCount() = 0
                // *q = r;     // localTrans.derefCount() = 1
                if (!es->interLinks.isEmpty() &&
                    localTrans.derefCount() <= es->lastLinkTrans.derefCount()) {
#ifdef DEBUG_POINTS_TO
                    debugmsg(QString("Local deref count is less or equal "
                                     "to last link's deref count, so it's "
                                     "no indirect assignment (%1 <= %2)"
                                     "at %3:%4:%5")
                             .arg(localTrans.derefCount())
                             .arg(es->lastLinkTrans.derefCount())
                             .arg(_ast ? _ast->fileName() : QString("-"))
                             .arg(es->root->start->line)
                             .arg(es->root->start->charPosition));
#endif
                    return assignments;
                }

                // Combine global with local transformations
                es->transformations = combineTansformations(
                            es->transformations, localTrans,
                            es->lastLinkTrans);

                // Must be a valid lvalue
                if (!es->validLvalue || es->transformations.derefCount() < 0)
                    return assignments;

                // Do not insert links for recursive expressions, we cannot
                // resolve them anyway.
                TransformedSymbol transSym(es->sym, es->transformations);
                if (_symbolsBelowNode[assigned].contains(es->sym, transSym))
                    return assignments;

                // Record assignments for variables and parameters
                if ((es->sym->type() & PointsToTypes) &&
                    es->srcNode->scope->varAssignment(
                        es->sym->name(), assigned, es->transformations,
                        _pointsToRound))
                {
#ifdef DEBUG_POINTS_TO
                    QString s;
                    if (!es->transformations.isEmpty())
                        s = QString(" with \"%1\"").arg(
                                    es->transformations.toString(
                                        es->sym->name()));
                    debugmsg(QString("Symbol \"%1\" at %2:%3 points to "
                                     "%4:%5:%6%7")
                             .arg(es->sym->name())
                             .arg(es->srcNode->start->line)
                             .arg(es->srcNode->start->charPosition)
                             .arg(ast_node_type_to_str(assigned))
                             .arg(assigned->start->line)
                             .arg(assigned->start->charPosition)
                             .arg(s));
#endif
                    ++_assignments;
                    ++assignments;
                }
                return assignments;
            }
            // Is this in the right-hand of an assignment expression?
            else if (es->root->u.assignment_expression.lvalue &&
               es->prevNode == es->root->u.assignment_expression.assignment_expression)
            {
                return assignments;
            }
            break;

        case nt_conditional_expression:
            // A conditional expression with '?' operator yields no valid lvalue
            if (es->root->u.conditional_expression.conditional_expression)
                es->validLvalue = false;
            break;

        case nt_init_declarator:
            // Is this in the left-hand of an init declarator?
            if (es->prevNode == es->root->u.init_declarator.declarator &&
                (assigned = es->root->u.init_declarator.initializer))
            {
                // We disallow following inter-links to the left-hand side of
                // initializations, this would make no sense
                if (!es->interLinks.isEmpty())
                    return assignments;

                // Combine global with local transformations. Acutally no need
                // to trim the transformations because es->lastLinkTrans is
                // guaranteed to be empty.
                es->transformations = combineTansformations(
                            es->transformations, localTrans,
                            es->lastLinkTrans);

                // Record assignments for variables and parameters
                if ((es->sym->type() & PointsToTypes) &&
                    es->srcNode->scope->varAssignment(
                        es->sym->name(), assigned, es->transformations,
                        _pointsToRound))
                {
#ifdef DEBUG_POINTS_TO
                    QString s;
                    if (!es->transformations.isEmpty())
                        s = QString(" with \"%1\"").arg(
                                    es->transformations.toString(
                                        es->sym->name()));
                    debugmsg(QString("Symbol \"%1\" at %2:%3 points to "
                                     "%4:%5:%6%7")
                             .arg(es->sym->name())
                             .arg(es->srcNode->start->line)
                             .arg(es->srcNode->start->charPosition)
                             .arg(ast_node_type_to_str(assigned))
                             .arg(assigned->start->line)
                             .arg(assigned->start->charPosition)
                             .arg(s));
#endif
                    ++_assignments;
                    ++assignments;
                }
                return assignments;
            }
            break;

        case nt_initializer:
            // Is this an array or struct initializer "struct foo f = {...}" ?
            if (es->root->parent && es->root->parent->type == nt_initializer)
                return assignments;
            break;

        case nt_jump_statement_return: {
            // An identifier underneath a return should be the initializer
            assert((assigned = es->root->u.jump_statement.initializer) != 0);

            // Check last link's transformations are a prefix of the local ones
            if (!es->lastLinkTrans.isPrefixOf(localTrans)) {
#ifdef DEBUG_POINTS_TO
                debugmsg(QString("Symbol \"%1\" at %2:%3 is no prefix of "
                                 "\"%4\" at %5:%6:%7")
                         .arg(es->lastLinkTrans.toString(es->sym->name()))
                         .arg(es->srcNode->start->line)
                         .arg(es->srcNode->start->charPosition)
                         .arg(localTrans.toString(es->sym->name()))
                         .arg(_ast ? _ast->fileName() : QString("-"))
                         .arg(es->root->start->line)
                         .arg(es->root->start->charPosition));
#endif
                return assignments;
            }

            const ASTSymbol* funcSym = embeddingFuncSymbol(es->root);
            // funcSym could be null here so return (although that triggers an assert)
            if(!funcSym) return assignments;
            // Do not insert links for recursive expressions, we cannot
            // resolve them anyway.
            if (_symbolsBelowNode[assigned].contains(funcSym))
                return assignments;

            // Treat any return statement as an assignment to the function
            // definition symbol with a function call transformations
            SymbolTransformations trans(this);
            trans.append(ttFuncCall, es->root);
            if (es->root->scope->varAssignment(
                    funcSym->name(), assigned, trans, _pointsToRound))
            {
#ifdef DEBUG_POINTS_TO
                debugmsg(QString("Function symbol \"%1\" at %2:%3 points to "
                                 "%4:%5:%6")
                         .arg(funcSym->name())
                         .arg(funcSym->astNode()->start->line)
                         .arg(funcSym->astNode()->start->charPosition)
                         .arg(ast_node_type_to_str(assigned))
                         .arg(assigned->start->line)
                         .arg(assigned->start->charPosition));
#endif
                ++_assignments;
                ++assignments;
            }
            }
            return assignments;

        case nt_postfix_expression:
            appendTransformations(es->root, &localTrans);
            break;

        case nt_unary_expression_op:
            appendTransformations(es->root, &localTrans);
            op = antlrTokenToStr(es->root->u.unary_expression.unary_operator);
            if (op != "*" && !op.startsWith("&"))
                es->validLvalue = false;
            break;

        // Types we have to skip to come to a decision
        case nt_builtin_function_choose_expr:
        case nt_cast_expression:
        case nt_declarator:
        case nt_direct_declarator:
        case nt_lvalue:
        case nt_primary_expression:
        case nt_unary_expression:
        case nt_unary_expression_builtin:
            break;

        //----------------------------------------------------------------------
        // Types for which we know we're done
        case nt_assembler_parameter:
        case nt_assembler_statement:
        case nt_builtin_function_alignof:
        case nt_builtin_function_constant_p:
        case nt_builtin_function_expect:
        case nt_builtin_function_extract_return_addr:
        case nt_builtin_function_object_size:
        case nt_builtin_function_offsetof:
        case nt_builtin_function_prefetch:
        case nt_builtin_function_return_address:
        case nt_builtin_function_sizeof:
        case nt_builtin_function_types_compatible_p:
        case nt_builtin_function_va_arg:
        case nt_builtin_function_va_copy:
        case nt_builtin_function_va_end:
        case nt_builtin_function_va_start:
        case nt_compound_braces_statement:
        case nt_compound_statement:
        case nt_constant_expression:
        case nt_declaration:
        case nt_declarator_suffix_brackets:
        case nt_designated_initializer:
        case nt_expression_statement:
        case nt_function_definition:
        case nt_iteration_statement_do:
        case nt_iteration_statement_for:
        case nt_iteration_statement_while:
        case nt_jump_statement_goto:
        case nt_labeled_statement:
        case nt_labeled_statement_case:
        case nt_labeled_statement_default:
        case nt_parameter_declaration:
        case nt_postfix_expression_brackets:
        case nt_postfix_expression_parens: /// @todo might be interesting
        case nt_selection_statement_if:
        case nt_selection_statement_switch:
        case nt_struct_declarator:
        case nt_typeof_specification:
        case nt_unary_expression_dec:
        case nt_unary_expression_inc:
            return assignments;

        default:
            typeEvaluatorError(QString("Unhandled node type \"%1\" in %2 at "
                                       "%3:%4:%5")
                               .arg(ast_node_type_to_str(es->root))
                               .arg(__PRETTY_FUNCTION__)
                               .arg(_ast->fileName())
                               .arg(es->root->start->line)
                               .arg(es->root->start->charPosition));
        }

        // Otherwise go up
        es->prevNode = es->root;
        es->root = es->root->parent;
    }

    return assignments;
}


void ASTTypeEvaluator::evaluateIdentifierPointsToRev(const ASTNode *node)
{
    if (!node)
        return;
    checkNodeType(node, nt_primary_expression);

    const ASTSymbol* sym = findSymbolOfPrimaryExpression(node, false);
    if (!sym)
        return;
    // For every node that has been assigned to sym, we insert an inverse entry
    // into the hash table
    for (AssignedNodeSet::const_iterator it =
            sym->assignedAstNodes().begin(),
            e = sym->assignedAstNodes().end();
         it != e; ++it)
    {
        // Only consider newly added links
        if (it->addedInRound == _pointsToRound) {
            AssignedNode an(sym, node, it->transformations, _pointsToRound);
            if (!_assignedNodesRev.contains(it->node, an))
                _assignedNodesRev.insertMulti(it->node, an);
        }
    }
}


ASTTypeEvaluator::EvalResult ASTTypeEvaluator::evaluateTypeFlow(
        TypeEvalDetails* ed)
{
    const ASTNode *lNode = 0, *rNode = ed->srcNode;
    ASTType *lType = 0;
    ASTNodeNodeMHash::const_iterator it, e = _assignedNodesRev.end();
    TypeEvalDetails rek_ed(this);
    SymbolTransformations localTrans(this);
    QString op;

    // Is this somewhere in the right-hand of an assignment expression or
    // of an init declarator?
    while (ed->rootNode) {
        // Beware of loops in recursion
        if (ed->interLinks.contains(ed->rootNode))
            return erRecursiveExpression;

        it = ed->followInterLinks ? _assignedNodesRev.find(ed->rootNode) : e;
        // Was this node assigned to other variables?
        if (it != e) {
            if (!interLinkTypeMatches(ed, localTrans))
                return erInvalidTransition;

            SymbolTransformations combTrans = combineTansformations(
                        ed->transformations, localTrans,
                        ed->lastLinkTrans);
            if (// Limit recursion level
                (ed->interLinks.size() < 4) &&
                // If last link involved transformations, they must be a prefix
                // of the local transformations
                ed->lastLinkTrans.isPrefixOf(localTrans) &&
                // Do not recurse for plain local variables without context
                // (i.e., postfix expression suffixes)
                (ed->sym->isGlobal() || combTrans.memberCount() > 0))
            {

                for (; it != e && it.key() == ed->rootNode; ++it) {
                    // Skip all inter-links that lead back to the source node
                    if (it->node == ed->srcNode)
                        continue;
                    // Skip all targets that are target of this symbol anyway
                    if (ed->sym->assignedAstNodes().contains(it.value()))
                        continue;

                    rek_ed = *ed;
                    rek_ed.rootNode = it->node;
                    rek_ed.lastLinkTrans = it->transformations;
                    rek_ed.lastLinkSrcType = typeofNode(ed->rootNode);
                    rek_ed.lastLinkDestType = typeofNode(
                                it->transformations.isEmpty() ?
                                    it->node :
                                    it->transformations.last().node);
                    rek_ed.transformations = combTrans;
                    rek_ed.interLinks.insert(ed->rootNode, rek_ed.rootNode);
                    evaluateIdentifierUsedAsRek(&rek_ed);
                }
            }
#ifdef DEBUG_USED_AS
            // It's not the same pointer of the deref counters don't not match!
            else {
                QString reason("of unknown reason");
                if (ed->interLinks.size() >= 4)
                    reason = QString("recursion level limit (%1) was reached ")
                            .arg(ed->interLinks.size());
                else if (!ed->lastLinkTrans.isPrefixOf(localTrans))
                    reason = QString("%1 is no prefix of %2")
                            .arg(ed->lastLinkTrans.toString(ed->sym->name()))
                            .arg(localTrans.toString(ed->sym->name()));
                else if (!ed->sym->isGlobal() && combTrans.memberCount() <= 0)
                    reason = QString("source symbol \"%1\" is local variable "
                                     "without suffixes")
                            .arg(ed->sym->name());

                debugmsg(QString("Skipping all links from %1:%2:%3 because %4")
                         .arg(ast_node_type_to_str(ed->rootNode))
                         .arg(ed->rootNode->start->line)
                         .arg(ed->rootNode->start->charPosition)
                         .arg(reason));
            }
#endif
        }

        // Find out the situation in which the identifier is used
        switch (ed->rootNode->type) {
        case nt_additive_expression:
            // Ignore numeric types in arithmetic expressions
            if (ed->rootNode->u.binary_expression.right &&  !ed->castExNode) {
                SymbolTransformations combTrans = combineTansformations(
                            ed->transformations, localTrans,
                            ed->lastLinkTrans);
                const ASTNode* n = localTrans.isEmpty() ?
                            ed->primExNode->parent : localTrans.last().node;
                if (typeofNode(n)->type() & NumericTypes)
                    return erIntegerArithmetics;
            }
            break;

        case nt_assignment_expression:
            // Is this in the right-hand of an assignment expression?
            if ((lNode = ed->rootNode->u.assignment_expression.lvalue) &&
                (rNode == ed->rootNode->u.assignment_expression.assignment_expression))
            {
                lType = typeofNode(lNode);
                goto while_exit;
            }
            else if (ed->rootNode->u.assignment_expression.designated_initializer_list &&
                (rNode == ed->rootNode->u.assignment_expression.initializer))
            {
                lType = typeofDesignatedInitializerList(
                            ed->rootNode->u.assignment_expression.designated_initializer_list);
                lNode = lType ? lType->node() : 0;
                goto while_exit;
            }
            break;

        case nt_assembler_parameter:
        case nt_assembler_statement:
            return erNoAssignmentUse;

        case nt_builtin_function_alignof:
//        case nt_builtin_function_choose_expr:
        case nt_builtin_function_constant_p:
        case nt_builtin_function_expect:
        case nt_builtin_function_extract_return_addr:
        case nt_builtin_function_object_size:
        case nt_builtin_function_offsetof:
        case nt_builtin_function_prefetch:
        case nt_builtin_function_return_address:
        case nt_builtin_function_sizeof:
        case nt_builtin_function_types_compatible_p:
        case nt_builtin_function_va_arg:
        case nt_builtin_function_va_copy:
        case nt_builtin_function_va_end:
        case nt_builtin_function_va_start:
            return erUseInBuiltinFunction;

        case nt_cast_expression:
            // Save the last actual casting cast expression for later reference,
            // but only if the cast actually changes the type
            if (ed->rootNode->u.cast_expression.type_name &&
                rNode == ed->rootNode->u.cast_expression.cast_expression &&
                !typeofNode(rNode)->equalTo(
                    typeofNode(ed->rootNode->u.cast_expression.type_name)))
                ed->castExNode = ed->rootNode;
            break;

        case nt_compound_braces_statement: {
            // If rNode was NOT the last expression within the list, then it's
            // of no interest
            struct ASTNodeList *l =
                    ed->rootNode->u.compound_braces_statement.declaration_or_statement_list;
            for (; l; l = l->next)
                if (l->item == rNode && l->next)
                    return erNoAssignmentUse;
            }
            break;

        case nt_compound_statement:
            return erNoAssignmentUse;

        case nt_conditional_expression:
            // Is this the condition in a conditional expression (.. ? .. : ..) ?
            if (ed->rootNode->u.conditional_expression.logical_or_expression == rNode &&
                ed->rootNode->u.conditional_expression.conditional_expression)
                return erNoAssignmentUse;
            break;

        case nt_declarator_suffix_brackets:
        case nt_designated_initializer:
            // Used as an array size or index value
            return erNoAssignmentUse;

        case nt_expression_statement:
            // Expressions might be the return value of compound braces
            // statements if they appear last
            if (ed->rootNode->parent->type != nt_compound_braces_statement)
                return erNoAssignmentUse;
            break;

        case nt_init_declarator:
            // Is this in the right-hand of an init declarator?
            if ((lNode = ed->rootNode->u.init_declarator.declarator) &&
               rNode == ed->rootNode->u.init_declarator.initializer)
            {
                lType = typeofNode(lNode);
                goto while_exit;
            }
            break;

        case nt_initializer:
            // Is this an array or struct initializer "struct foo f = {...}" ?
            if (ed->rootNode->parent && ed->rootNode->parent->type == nt_initializer) {
                lType = expectedTypeAtInitializerPosition(ed->rootNode);
                lNode = lType ? lType->node() : 0;
                goto while_exit;
            }
            break;

        case nt_iteration_statement_do:
        case nt_iteration_statement_for:
        case nt_iteration_statement_while:
            return erNoAssignmentUse;

        case nt_jump_statement_goto:
            return erNoAssignmentUse;

        case nt_jump_statement_return:
            // An identifier underneath a return should be the initializer
            assert(ed->rootNode->u.jump_statement.initializer != 0);
            lType = embeddingFuncReturnType(ed->rootNode);
            lNode = lType ? lType->node() : 0;

            goto while_exit;

        case nt_postfix_expression:
            if (ed->rootNode->u.postfix_expression.postfix_expression_suffix_list) {
                // Postfix expression suffixes may represent a type usage if the
                // type has already been casted. In that case, this is the first
                // effective type change
                if (ed->castExNode)
                    goto cast_expression_type_change;
                else
                    localTrans.append(ed->rootNode->u.postfix_expression
                                      .postfix_expression_suffix_list);
            }
            break;

        case nt_postfix_expression_brackets:
            // Used as array index
        case nt_postfix_expression_parens:
            /// @todo Use as a function parameter in nt_postfix_expression_parens might be interesting!
            return erNoAssignmentUse;

        case nt_selection_statement_if:
        case nt_selection_statement_switch:
            return erNoAssignmentUse;

        case nt_typeof_specification:
            return erNoAssignmentUse;

        // Unary logical expression
        case nt_unary_expression_op:
            op = antlrTokenToStr(ed->rootNode->u.unary_expression.unary_operator);
            // The negation operator results in a boolean expression
            if ("!" == op)
                return erNoAssignmentUse;
            if ("&" == op) {
                // See if this address operator was skipped
                ASTType* type = typeofNode(ed->rootNode);
                if (!type->ampersandSkipped())
                    localTrans.append(ttAddress, ed->rootNode);
            }
            else if ("&&" == op) {
                // See if this address operator was skipped
                ASTType* type = typeofNode(ed->rootNode);
                if (!type->ampersandSkipped())
                    localTrans.append(ttAddress, ed->rootNode);
                localTrans.append(ttAddress, ed->rootNode);
            }
            // Dereferencing is a type usage, so if the type has already been
            // casted, this is the first effective type change
            else if ("*" == op) {
                if (ed->castExNode)
                    goto cast_expression_type_change;
                else
                    localTrans.append(ttDereference, ed->rootNode);
            }
            break;

        // Binary logical expressions
        case nt_equality_expression:
        case nt_logical_and_expression:
        case nt_logical_or_expression:
        case nt_relational_expression:
            // A binary expression with a right-hand side returns a boolean
            if (ed->rootNode->u.binary_expression.right)
                return erNoAssignmentUse;
            break;

        default:
            break;
        }

        // Otherwise go up
        rNode = ed->rootNode;
        ed->rootNode = ed->rootNode->parent;
    }

    goto while_exit;

    cast_expression_type_change:
    ed->rootNode = ed->castExNode;
    lNode = ed->castExNode->u.cast_expression.type_name;
    rNode = ed->castExNode->u.cast_expression.cast_expression;
    lType = typeofNode(lNode);

    while_exit:

    ed->targetNode = lNode;
    ed->targetType = lType;

    // No interesting case, so we're done
    if (!ed->rootNode)
        return erNoAssignmentUse;

    // Check if local transformations are a prefix of last
    // link's transformations
    if (!ed->lastLinkTrans.isPrefixOf(localTrans)) {
#ifdef DEBUG_USED_AS
        if (ed->followInterLinks) {
            debugmsg(QString("Skipping change in %1 %2:%3 because %4 is no "
                             "prefix of %5")
                     .arg(ast_node_type_to_str(ed->rootNode))
                     .arg(ed->rootNode->start->line)
                     .arg(ed->rootNode->start->charPosition)
                     .arg(ed->lastLinkTrans.toString(ed->sym->name()))
                     .arg(localTrans.toString(ed->sym->name())));
        }
#endif
        return erInvalidTransition;
    }

    // If we followed a link for which the type changed AND we have futher
    // symbol transformations, the transformations do not fit to the changed
    // type
    if (!interLinkTypeMatches(ed, localTrans))
        return erInvalidTransition;

    // Combine the transformations
    ed->transformations = combineTansformations(
                ed->transformations, localTrans,
                ed->lastLinkTrans);

    // A negative deref counter means a net address operator for which
    // we cannot record the type changes
    if (ed->transformations.derefCount() < 0) {
#ifdef DEBUG_USED_AS
        if (ed->followInterLinks) {
            debugmsg(QString("Skipping change in %1 %2:%3 because of a negative "
                             "deref counter (%4)")
                     .arg(ast_node_type_to_str(ed->rootNode))
                     .arg(ed->rootNode->start->line)
                     .arg(ed->rootNode->start->charPosition)
                     .arg(ed->transformations.derefCount()));
        }
#endif
        return erAddressOperation;
    }

    // Skip if source and target types are equal
    const ASTNode* n = ed->transformations.isEmpty() ?
                ed->primExNode->parent : ed->transformations.last().node;
    if (lType->equalTo(typeofNode(n)))
        return erTypesAreEqual;

    ASTType* rType = typeofNode(rNode);
    // Skip if resulting type of one side does not fit to the other side. We
    // allow pointer-to-integer casts, though.
    if (lType->isPointer() &&
        !rType->isPointer() &&
        !(sizeofPointer() == 4 && (rType->type() & (rtUInt32|rtInt32))) &&
        !(sizeofPointer() == 8 && (rType->type() & (rtUInt64|rtInt64))))
        return erNoPointerAssignment;
    if (rType->isPointer() &&
        !lType->isPointer() &&
        !(sizeofPointer() == 4 && (lType->type() & (rtUInt32|rtInt32))) &&
        !(sizeofPointer() == 8 && (lType->type() & (rtUInt64|rtInt64))))
        return erNoPointerAssignment;

    return erTypesAreDifferent;
}


bool ASTTypeEvaluator::interLinkTypeMatches(
        const TypeEvalDetails* ed,
        const SymbolTransformations& localTrans) const
{
    // If last link changed the type AND we have futher symbol
    // transformations, they do not fit to the changed type
    if (!ed->interLinks.isEmpty() &&
        ed->lastLinkTrans.size() != localTrans.size() &&
        !ed->lastLinkSrcType->equalTo(ed->lastLinkDestType))
    {
        // Do we have type-specific local transformations?
        for (int i = ed->lastLinkTrans.size(); i < localTrans.size(); ++i)
            if (localTrans[i].type & (ttMember|ttArray))
                return false;
    }
    return true;
}

typedef QList<ASTType*> TypeChain;


ASTTypeEvaluator::EvalResult ASTTypeEvaluator::evaluateTypeChanges(
        TypeEvalDetails* ed)
{
    // Find the chain of changing types up to ae on the right-hand
    TypeChain typeChain;
    typeChain.append(typeofNode(ed->srcNode));
    int forcedChanges = 0, localDeref = 0;
    const ASTNode* p = ed->srcNode->parent;
    ASTNodeNodeHash interLinks = ed->interLinks;
    QString op;

    while (p != ed->rootNode) {
        ASTType* t = typeofNode(p);
        assert(t != 0);

        if (!t->equalTo(typeChain.last())) {
            // Is this an expected change of types?
            bool forced = true;
            switch(p->type) {
            case nt_additive_expression:
                // Type changes through additive expressions are never forced.
                forced = false;
                break;

            case nt_postfix_expression:
                // Member access or array operator?
                if (p->u.postfix_expression.postfix_expression_suffix_list)
                    forced = false;
                break;

            case nt_unary_expression_op:
                op = antlrTokenToStr(p->u.unary_expression.unary_operator);
                // Pointer dereferencing?
                if (op == "*" &&
                        (typeChain.last()->type() & (rtArray|rtPointer)) &&
                        t->equalTo(typeChain.last()->next()))
                {
                    ++localDeref;
                    forced = false;
                }
                // Address operator?
                else if (op == "&" && (t->type() & rtPointer) &&
                         typeChain.last()->equalTo(t->next()))
                {
                    --localDeref;
                    forced = false;
                }
                else if (op == "&&" && t->next()->next() &&
                         (t->type() & rtPointer) &&
                         (t->next()->type() & rtPointer) &&
                         typeChain.last()->equalTo(t->next()->next()))
                {
                    localDeref -= 2;
                    forced = false;
                }
                break;

            default:
                break;
            }

            typeChain.append(t);
            if (forced) {
                ++forcedChanges;
#ifdef DEBUG_USED_AS
                debugmsg(QString("Added type %1 (%2) to chain")
                         .arg(typeChain.last()->toString())
                         .arg(typeChain.last()->node() ?
                                  QString("%1:%2")
                                  .arg(typeChain.last()->node()->start->line)
                                  .arg(typeChain.last()->node()->start->charPosition) :
                                  QString("builtin")));
#endif
            }
        }

        // Follow the inter-connecting links, if existent, otherwise go up
        if (interLinks.contains(p)) {
#ifdef DEBUG_USED_AS
            QString s = QString("Following inter-link %1:%2:%3 => %4:%5:%6")
                    .arg(ast_node_type_to_str(p))
                    .arg(p->start->line)
                    .arg(p->start->charPosition)
                    .arg(ast_node_type_to_str(ed->interLinks[p]))
                    .arg(ed->interLinks[p]->start->line)
                    .arg(ed->interLinks[p]->start->charPosition);
            if (ed->interLinks[p]->type == nt_primary_expression)
                s += QString(" (%1)").arg(
                            findSymbolOfPrimaryExpression(ed->interLinks[p])->name());
            debugmsg(s);
#endif
            const ASTNode* tmp = p;
            p = interLinks[p];
            // Follow every inter-link at most once
            interLinks.remove(tmp);
        }
        else
            p = p->parent;
    }

    // Skip if both sides have the same, single type
    if (forcedChanges == 0) {
        if (ed->targetType->equalTo(typeChain.last()) ||
            ed->targetType->equalTo(typeChain.first()))
        {
#ifdef DEBUG_USED_AS
            debugmsg(QString("Line %1: Skipping because types are equal (%1:%2)")
                        .arg(ed->rootNode->start->line)
                        .arg(ed->rootNode->start->charPosition));
#endif
            return erTypesAreEqual;
        }
        // Check for pointer arithmetics with += or -= operator
        else if (ed->rootNode->type == nt_assignment_expression) {
            QString op = antlrTokenToStr(
                        ed->rootNode->u.assignment_expression.assignment_operator);

            // Treat "+="/"-=" as lvalue = lvalue +/- assignment_expression
            if (op != "=") {
                ASTType *rType = typeofNode(
                            ed->rootNode->u.assignment_expression.assignment_expression);
                ASTType *resType =  (op == "+=" || op == "-=") ?
                            typeofAdditiveExpression(ed->targetType, rType, op) :
                            typeofIntegerExpression(ed->targetType, rType, op);

                if (ed->targetType->equalTo(resType)) {
#ifdef DEBUG_USED_AS
                    debugmsg(QString("Line %1: Skipping because types are equal (%1:%2)")
                             .arg(ed->rootNode->start->line)
                             .arg(ed->rootNode->start->charPosition));
#endif
                    return erTypesAreEqual;
                }
                else {
                    typeChain.append(resType);
                    ++forcedChanges;
#ifdef DEBUG_USED_AS
                    debugmsg(QString("Added type %1 (%2) to chain")
                             .arg(typeChain.last()->toString())
                             .arg(typeChain.last()->node() ?
                                      QString("%1:%2")
                                      .arg(typeChain.last()->node()->start->line)
                                      .arg(typeChain.last()->node()->start->charPosition) :
                                      QString("builtin")));
#endif
                }
            }
        }
    }

    // Skip if right-hand side is dereferenced and assigned to compatible
    // lvalue
    if (localDeref) {
        const ASTType* lt = ed->targetType;
        const ASTType* rt = typeChain.first();
        // For all (de)references, skip one pointer of the corresponding type
        int i = (localDeref < 0) ? -localDeref : localDeref;
        for (; i > 0; --i) {
            // Positive deref: dereference right side
            if (localDeref > 0) {
                if (rt && (rt->type() & (rtPointer|rtArray)))
                    rt = rt->next();
                else
                    break;
            }
            // Negative deref: dereference left side
            else {
                if (lt && (lt->type() & (rtPointer|rtArray)))
                    lt = lt->next();
                else
                    break;
            }
        }

        // Now compare the types
        if (i == 0 && lt->equalTo(rt))
            return erTypesAreEqual;
    }

    // See if the right side involves pointers
    bool rHasPointer = false;
    for (int i = 0; !rHasPointer && i < typeChain.size(); ++i)
        rHasPointer = typeChain[i]->isPointer();

    // Skip if neither the left nor the right side involves pointers
    if (!rHasPointer && !ed->targetType->isPointer()) {
#ifdef DEBUG_USED_AS
        debugmsg(QString("Line %1: Skipping because no pointer "
                "assignment (%1:%2)")
                .arg(ed->rootNode->start->line)
                .arg(ed->rootNode->start->charPosition));
#endif
        return erNoPointerAssignment;
    }

    // iS left-hand type different?
    if (!ed->targetType->equalTo(typeChain.last())) {
//        typeChain.append(ed->targetType);
        ++forcedChanges;
#ifdef DEBUG_USED_AS
        debugmsg(QString("Added type %1 (%2) to chain")
                 .arg(typeChain.last()->toString())
                 .arg(typeChain.last()->node() ?
                          QString("%1:%2")
                          .arg(typeChain.last()->node()->start->line)
                          .arg(typeChain.last()->node()->start->charPosition) :
                          QString("builtin")));
#endif
    }

    return (forcedChanges > 0) ? erTypesAreDifferent : erTypesAreEqual;
}


void ASTTypeEvaluator::evaluateTypeContext(TypeEvalDetails* ed)
{
    // Embedding struct in whose context we see the type change
    ed->ctxNode = ed->primExNode;
    ed->ctxTransformations = ed->transformations;
    // Source node is the last suffix node of the last postfix expression
    ed->srcNode = (ed->transformations.isEmpty()) ?
                ed->primExNode : ed->transformations.last().node;
    ed->srcType = typeofNode(ed->srcNode);
    // Member chain of embedding struct in whose context we see the type change
    // True as long as we see member.sub.subsub expressions
    bool searchMember = true;
    // Operations to be performed on the resulting ctxType
    QStack<SymbolTransformationType> ctxTypeOps;

    // Go through all postfix expression suffixes from right to left
    SymbolTransformations::const_iterator it = ed->transformations.end();
    SymbolTransformations::const_iterator begin = ed->transformations.begin();
    int memberCnt = 0;
    while (it-- != begin) {
        // Still building chain of members?
        if (searchMember) {
            switch(it->type) {
            case ttMember:
                // Type changes here, so clear all type operations
                ctxTypeOps.clear();
                ++memberCnt;
                break;

            case ttDereference:
                // Type changes now, so clear all pending operations
                ctxTypeOps.clear();
                ctxTypeOps.push(it->type);

                // Stop searching members if we found one already
                if (memberCnt)
                    searchMember = false;
                break;

            case ttArray: {
//                ctxTypeOps.push(it->type);
                // If brackets are used on a pointer type, then this is a
                // dereference and the next suffix is the context type,
                // but for embedded array types we treat it the same way as a
                // "dot" member access
                const ASTNode* pred;
                if (it == begin) {
                    checkNodeType(it->node->parent, nt_postfix_expression);
                    pred = it->node->parent
                            ->u.postfix_expression.primary_expression;
                }
                else {
                    SymbolTransformations::const_iterator tmp = it - 1;
                    pred = tmp->node;
                }
                if (typeofNode(pred)->type() == rtArray)
                    ctxTypeOps.push(it->type);
                else {
                    // Type changes now, so clear all pending operations
                    ctxTypeOps.clear();
                    // The brackets are by themselves a dereference
                    ctxTypeOps.push(it->type);
                    // Stop searching members if we found one already
                    if (memberCnt)
                    searchMember = false;
                }
                break;
            }

            case ttAddress:
            case ttFuncCall:
                ctxTypeOps.push(it->type);
                break;

            case ttNull:
                break;
            }
        }
        else {
            // This is the node of the embedding struct
            ed->ctxNode = it->node;
            // The context transformations are all that we have visited so far
            int len = (ed->transformations.constEnd() - it) - 1;
            ed->ctxTransformations = ed->transformations.right(len);
            break;
        }
    }

    // The context node is found, now evaluate its type
    ed->ctxType = typeofNode(ed->ctxNode);

    // Perform pending operations on the context type
    while (!ctxTypeOps.isEmpty()) {
        SymbolTransformationType type = ctxTypeOps.pop();
        switch (type) {
        case ttDereference:
        case ttArray:
            // Array operator, i.e., dereferencing
            if (!ed->ctxType || !(ed->ctxType->type() & (rtFuncPointer|rtPointer|rtArray)))
                typeEvaluatorError2(*ed,
                        QString("Expected a pointer or array type here instead "
                                "of \"%1\" at %2:%3:%4")
                                .arg(ed->ctxType ? ed->ctxType->toString() : QString())
                                .arg(_ast->fileName())
                                .arg(ed->ctxNode->start->line)
                                .arg(ed->ctxNode->start->charPosition));
            // Remove top pointer/array type
            ed->ctxType = ed->ctxType->next();
            break;

        case ttFuncCall:
            // Function operator, i.e., function call
            if (!ed->ctxType || !(ed->ctxType->type() & (rtFuncPointer)))
                typeEvaluatorError2(*ed,
                        QString("Expected a function pointer type here instead "
                                "of \"%1\" at %2:%3:%4")
                                .arg(ed->ctxType ? ed->ctxType->toString() : QString())
                                .arg(_ast->fileName())
                                .arg(ed->ctxNode->start->line)
                                .arg(ed->ctxNode->start->charPosition));
            // Remove top function pointer type
            ed->ctxType = ed->ctxType->next();
            break;

        case ttAddress:
            ed->ctxType = createASTType(rtPointer, ed->ctxType);
            break;

        case ttMember:
        case ttNull:
            // ignored
            break;
        }
    }

    if (!ed->ctxType || !ed->ctxNode)
        typeEvaluatorError2(*ed, QString("Either context type or context node is "
                "null: ctxType = 0x%1, ctxNode = 0x%2")
                .arg((quint64)ed->ctxType, 0, 16)
                           .arg((quint64)ed->ctxNode, 0, 16));
}


SymbolTransformations ASTTypeEvaluator::combineTansformations(
        const SymbolTransformations &global, const SymbolTransformations &local,
        const SymbolTransformations &lastLink) const
{
    SymbolTransformations result(global);
    result.append(local.right(local.size() - lastLink.size()));
    return result;
}


ASTTypeEvaluator::EvalResult ASTTypeEvaluator::evaluateIdentifierUsedAs(
        const ASTNode* node)
{
    // We are only interested in primary expressions
    if (!node || node->type != nt_primary_expression)
    	return erNoPrimaryExpression;

    TypeEvalDetails ed(this);
    // Skip non-variable expressions
    if ( !(ed.sym = findSymbolOfPrimaryExpression(node, false)) )
        return erNoPrimaryExpression;

    // Ignore enumerator symbols, they are constant values
    if (ed.sym->type() == stEnumerator)
        return erNoPointerAssignment;

    ed.srcNode = node;
    ed.primExNode = node;
    ed.rootNode = node->parent;


#ifdef DEBUG_USED_AS
    debugmsg(QString("Starting with used-as analysis for \"%1\" at %2:%3")
             .arg(findSymbolOfPrimaryExpression(ed.primExNode)->name())
             .arg(ed.primExNode->start->line)
             .arg(ed.primExNode->start->charPosition));
#endif

    return evaluateIdentifierUsedAsRek(&ed);
}


ASTTypeEvaluator::EvalResult ASTTypeEvaluator::evaluateIdentifierUsedAsRek(
        TypeEvalDetails *ed)
{
    ASTTypeEvaluator::EvalResult ret;

    // Check for endless recursions
    if (ed->evalNodeStack.contains(ed->rootNode))
        return erRecursiveExpression;
    // Push current root on the recursion tracking stack, gets auto-popped later
    StackAutoPopper<ASTNodeStack> autoPopper(&ed->evalNodeStack, ed->rootNode);

    // Evaluate if type changes in this expression
    ret = evaluateTypeFlow(ed);

#ifdef DEBUG_USED_AS
    QString s;
    for (ASTNodeNodeHash::const_iterator it = ed->interLinks.begin();
         it != ed->interLinks.end(); ++it)
    {
        s += QString("\n    %1:%2:%3 => %4:%5:%6")
                .arg(ast_node_type_to_str(it.key()))
                .arg(it.key()->start->line)
                .arg(it.key()->start->charPosition)
                .arg(ast_node_type_to_str(it.value()))
                .arg(it.value()->start->line)
                .arg(it.value()->start->charPosition);
        if (it.value()->type == nt_primary_expression) {
            s += QString(" (%1)").arg(
                        findSymbolOfPrimaryExpression(it.value())->name());
        }
    }

    debugmsg(QString("Evaluating used-as for \"%1\" at %2:%3 (links: %4)%5")
             .arg(findSymbolOfPrimaryExpression(ed->primExNode)->name())
             .arg(ed->primExNode->start->line)
             .arg(ed->primExNode->start->charPosition)
             .arg(ed->interLinks.size())
             .arg(s));
#endif

    if (ret == erTypesAreDifferent) {

        // Evaluate the context of type change
        evaluateTypeContext(ed);

        if (ed->ctxType->type() == rtVoid)
            ret = erInvalidTransition;
        else {
            // Evaluate the chain of changing types
            ret = evaluateTypeChanges(ed);

            if (ret == erTypesAreDifferent)
                primaryExpressionTypeChange(*ed);
        }
    }

#ifdef DEBUG_USED_AS
    switch (ret) {
    case erNoPrimaryExpression:     s = "NoPrimaryExpression"; break;
    case erNoIdentifier:            s = "NoIdentifier"; break;
    case erUseInBuiltinFunction:    s = "UseInBuiltinFunction"; break;
    case erNoAssignmentUse:         s = "NoAssignmentUse"; break;
    case erNoPointerAssignment:     s = "NoPointerAssignment"; break;
    case erIntegerArithmetics:      s = "IntegerArithmetics"; break;
    case erTypesAreEqual:           s = "TypesAreEqual"; break;
    case erTypesAreDifferent:       s = "TypesAreDifferent"; break;
    case erLeftHandSide:            s = "LeftHandSide"; break;
    case erAddressOperation:        s = "AddressOperation"; break;
    case erRecursiveExpression:     s = "RecursiveExpression"; break;
    case erInvalidTransition:       s = "InvalidTransition"; break;
    }

    debugmsg(QString(" Result of used-as for \"%1\" at %2:%3 (links: %4): %5")
             .arg(findSymbolOfPrimaryExpression(ed->primExNode)->name())
             .arg(ed->primExNode->start->line)
             .arg(ed->primExNode->start->charPosition)
             .arg(ed->interLinks.size())
             .arg(s));
#endif

    return ret;
}


inline bool srcLineLessThan(const ASTNode* n1, const ASTNode* n2)
{
    if (n1->start->line == n2->start->line)
        return n1->start->charPosition < n2->start->charPosition;
    else
        return n1->start->line < n2->start->line;
}


QString ASTTypeEvaluator::typeChangeInfo(const TypeEvalDetails &ed,
                                         const QString& expr) const
{
    ASTSourcePrinter printer(_ast);
#   define INDENT "    "
    QString scope;
    if (ed.sym && (ed.sym->type() == stVariableDef ||
                   ed.sym->type() == stVariableDecl))
        scope = ed.sym->isGlobal() ? "global " : "local ";

    const ASTNode* srcNode = ed.primExNode ? ed.primExNode->parent : 0;
    if (!ed.transformations.isEmpty()) {
        srcNode = ed.transformations.last().node;
        if (srcNode && srcNode->parent->type == nt_postfix_expression)
            srcNode = srcNode->parent;
    }
    QString src = printer.toString(srcNode, false).trimmed();

    // Print the source of all of all inter-connected code snippets
    QString conSrc;
    QList<const ASTNode*> nodes = ed.interLinks.keys();
    qSort(nodes.begin(), nodes.end(), srcLineLessThan);
    for (int i = 0; i < nodes.size(); ++i) {
        conSrc += INDENT;
        conSrc += printer.toString(nodes[i]->parent, true);
    }

    QString e;
    if (!expr.isEmpty())
        e = QString(INDENT "Expr.:  %1\n").arg(expr);

    QString symName = ed.sym ? ed.sym->name() : QString();

    return QString(INDENT "Symbol: %1 (%2)\n"
                   INDENT "Lvalue: %3\n"
                   INDENT "Source: %4 %5\n"
                   INDENT "Target: %6 %7\n"
                   "%8"
                   "%9"
                   INDENT "%10")
            .arg(symName, -30)
            .arg(scope + (ed.sym ? ed.sym->typeToString() : QString()))
            .arg(ed.transformations.toString(symName))
            .arg(src + ",", -30)
            .arg(ed.srcType ? ed.srcType->toString() : QString())
            .arg(printer.toString(ed.targetNode, false).trimmed() + ",", -30)
            .arg(ed.targetType ? ed.targetType->toString() : QString())
            .arg(e)
            .arg(conSrc)
            .arg(printer.toString(ed.rootNode, true).trimmed());
}


void ASTTypeEvaluator::primaryExpressionTypeChange(const TypeEvalDetails &ed)
{
    checkNodeType(ed.primExNode, nt_primary_expression);

    QString symScope = ed.sym->isLocal() ? "local" : "global";
    QStringList symType = ed.sym->typeToString().split(' ');
    if (symType.last().startsWith('('))
    	symType.pop_back();

    ASTSourcePrinter printer(_ast);
    QString var;
    if (ed.transformations.isEmpty())
        var = postfixExpressionToStr(ed.primExNode->parent, ed.ctxNode);
    else {
        const ASTNode* n = ed.transformations.last().node;
        if (n->parent->type == nt_postfix_expression)
            n = n->parent;
        var = printer.toString(n).trimmed();
    }

    std::cout
            << (_ast && !_ast->fileName().isEmpty() ?
                    _ast->fileName() + ":%1:%2: " :
                    QString("Line %1:%2: "))
                .arg(ed.srcNode->start->line)
                .arg(ed.srcNode->start->charPosition)
            << ed.ctxType->toString()
            << ed.transformations.toString()
            << " is used as " << ed.targetType->toString()
            << " via " << symScope  << " " << symType.join(" ") << " "
            << "\"" << ed.sym->name() << "\"";

    if (ed.sym->name() != var)
        std::cout << " in \"" << var << "\"";

    std::cout << std::endl
              << typeChangeInfo(ed).toStdString()
              << std::endl;
}


ASTType* ASTTypeEvaluator::typeofTypeId(const ASTNode *node)
{
	if (!node)
	    return 0;
	checkNodeType(node, nt_type_id);

	QString name = antlrTokenToStr(node->u.type_id.identifier);

	const ASTSymbol* s = node->scope->find(name, ASTScope::ssTypedefs);

	if (!s || s->type() != stTypedef) {
	    typeEvaluatorError(
	            QString("Failed to resolve type \"%1\" at %2:%3")
	            .arg(name)
	            .arg(_ast->fileName())
	            .arg(node->start->line));
	}

	// Find node with given name in init_declarator_list
	const ASTNode* ddec = findIdentifierInIDL(
    		name,
            s->astNode()->u.declaration.init_declarator_list);

	if (!typeofNode(ddec)) {
        typeEvaluatorError(
                QString("Failed to resolve type \"%1\" at %2:%3")
                .arg(name)
                .arg(_ast->fileName())
                .arg(node->start->line));
	}

	return _types[ddec];
}


RealType ASTTypeEvaluator::evaluateBuiltinType(const pASTTokenList list,
											   QString* pTokens) const
{
	if (!list)
		return rtUndefined;

	enum Flags
	{
		fLong     = (1 << 0),
		fLongLong = (1 << 1),
		fShort    = (1 << 2),
		fSigned   = (1 << 3),
		fUnsigned = (1 << 4)
	};

	enum Types
	{
		tVoid    = (1 << 0),
		tChar    = (1 << 1),
		tBool    = (1 << 2),
		tInt     = (1 << 3),
		tFloat   = (1 << 4),
		tDouble  = (1 << 5),
		tVaList  = (1 << 6)
	};

	int types = 0, flags = 0;
	QString tokens;

	for (pASTTokenList p = list; p; p = p->next) {
		QString tok = antlrTokenToStr(p->item);

		if (!tokens.isEmpty())
			tokens += " ";
		tokens += tok;

		// Types
		if (tok == "void")
			types |= tVoid;
		else if (tok == "char")
			types |= tChar;
		else if (tok == "_Bool")
			types |= tBool;
		else if (tok == "int")
			types |= tInt;
		else if (tok == "float")
			types |= tFloat;
		else if (tok == "double")
			types |= tDouble;
		else if (tok == "__builtin_va_list")
			types |= tVaList;
		// Flags
		else if (tok == "long") {
			if (flags & fLong)
				flags |= fLongLong;
			else
				flags |= fLong;
		}
		else if (tok == "short")
			flags |= fShort;
		else if (tok == "signed" || tok == "__signed__")
			flags |= fSigned;
		else if (tok == "unsigned")
			flags |= fUnsigned;
		else
	        typeEvaluatorError(
	                QString("Unknown token \"%1\" for builtin type at %2:%3")
	                .arg(tok)
	                .arg(_ast->fileName())
	                .arg(p->item->line));
	}

	// default type is int
	if (!types)
		types |= tInt;

	if (types & tVoid)
		return rtVoid;

	if (types & tChar) {
		if (pTokens)
			*pTokens = (flags & fUnsigned) ? "unsigned char" : "char";
		return (flags & fUnsigned) ? rtUInt8 : rtInt8;
	}

	if (types & tBool) {
		if (pTokens)
			*pTokens = tokens;
		return rtBool8;
	}

	if (types & tInt) {
		if (flags & fShort) {
			if (pTokens)
				*pTokens = (flags & fUnsigned) ? "short unsigned int" : "short int";
			return (flags & fUnsigned) ? rtUInt16 : rtInt16;
		}
		if (flags & fLongLong) {
			if (pTokens)
				*pTokens = (flags & fUnsigned) ? "long long unsigned int" : "long long int";
			return (flags & fUnsigned) ? rtUInt64 : rtInt64;
		}
		if (flags & fLong) {
			if (pTokens)
				*pTokens = (flags & fUnsigned) ? "long unsigned int" : "long int";
			return (flags & fUnsigned) ? realTypeOfULong() : realTypeOfLong();
		}
		else {
			if (pTokens)
				*pTokens = (flags & fUnsigned) ? "unsigned int" : "int";
			return (flags & fUnsigned) ? rtUInt32 : rtInt32;
		}
	}

	if (types & tFloat)
		return rtFloat;

	if (types & tDouble) {
		if (flags & fLong)
			debugerr("In " << _ast->fileName() << ":" << list->item->line
					 << ": Encountered type \"long double\" which we may not "
						"handle correctly!");
		return rtDouble;
	}

	if (types & tVaList)
		return rtVaList;

    typeEvaluatorError(
            QString("Cannot resolve base type for \"%1\" at %2:%3")
            .arg(tokens)
            .arg(_ast->fileName())
            .arg(list->item->line));

	return rtUndefined;
}


void ASTTypeEvaluator::genDotGraphForNode(const ASTNode *node) const
{
    ASTDotGraph dg;
    QString s = _ast->fileName().split('/', QString::SkipEmptyParts).last();
    s = s.split('.').first();
    QString dotFile = QString("graph_%1_%2.dot")
            .arg(s)
            .arg(node->start->line);
    int nodes = dg.writeDotGraph(node, dotFile);

    debugmsg("Created file " << dotFile);

    // Invoke dot if no more than 1k nodes
    if (nodes > 0 && nodes < 1000) {
        QString pngFile = dotFile.split('.').first() + ".png";

        debugmsg("Generating graph image");
        QProcess proc;
        QString cmd("/usr/bin/dot");
        QStringList args;
        args << "-o" + pngFile << dotFile;

        proc.start(cmd, args);
        proc.waitForFinished(-1);
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
            debugerr("Running " << cmd << "failed, error code is " << proc.exitCode() << ".");
            return;
        }

        debugmsg("Launching image viewer");
        proc.reset();
        args.clear();
        cmd = "/usr/bin/gwenview";
        args << pngFile;

        proc.start(cmd, args);
        proc.waitForFinished(-1);
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
            debugerr("Running " << cmd << "failed, error code is " << proc.exitCode() << ".");
    }
}


int ASTTypeEvaluator::evaluateIntExpression(const ASTNode* /*node*/, bool* ok)
{
    // The default implementation does nothing.
    if (ok)
        *ok = false;
    return 0;
}


int ASTTypeEvaluator::stringLength(const ASTTokenList *list)
{
    if (!list)
        return -1;

    int len = 1; // for terminal '\0'
    QString s;
    const QString dot(".");
    QRegExp re("\\\\\\\\|\\\\(?:[abtnfreEv\"]|x[0-9a-fA-F]+|[0-7]{1,3})");
    for (; list; list = list->next) {
        s = antlrTokenToStr(list->item);
        // Remove outer quotes
        s = s.mid(1, s.length() - 2);
        // Replace escape sequences with a single character
        s = s.replace(re, dot);
        len += s.length();
    }

    return len;
}


bool ASTTypeEvaluator::interrupted() const
{
    return false;
}

