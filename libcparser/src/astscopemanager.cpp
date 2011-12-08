/*
 * myscopemanager.cpp
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#include <ast_interface.h>
#include <astscopemanager.h>
#include <debug.h>

ASTScope::~ASTScope()
{
	ASTSymbolHash::iterator it, e;
	// Delete all data
	for (it = _symbols.begin(), e = _symbols.end(); it != e; ++it)
		delete it.value();
	_symbols.clear();
	for (it = _compoundTypes.begin(), e = _compoundTypes.end(); it != e; ++it)
		delete it.value();
	_compoundTypes.clear();
	for (it = _typedefs.begin(), e = _typedefs.end(); it != e; ++it)
		delete it.value();
	_typedefs.clear();
}


void ASTScope::add(const QString &name, ASTSymbolType type, ASTNode *node)
{
	switch (type) {
	case stNull:
		debugerr("Trying to add symbol \"" << name << "\" with empty type!");
		break;
	case stEnumerator:
	case stFunctionDecl:
	case stFunctionDef:
	case stFunctionParam:
	case stStructMember:
	case stVariableDecl:
	case stVariableDef:
		addSymbol(name, type, node);
		break;
	case stEnumDecl:
	case stEnumDef:
	case stStructOrUnionDecl:
	case stStructOrUnionDef:
		addCompoundType(name, type, node);
		break;
	case stTypedef:
		addTypedef(name, type, node);
		break;
	}
}


bool ASTScope::varAssignment(const QString &name, const ASTNode *assignedNode,
                             const ASTNodeList* postExprSuffixes,
                             int derefCount, int round)
{
    // Search for variables, parameters and functions with given name
    for (ASTScope* p = this; p; p = p->_parent) {
        if (p->_symbols.contains(name) &&
            (p->_symbols[name]->type() &
             (stVariableDecl|stVariableDef|stFunctionParam|stFunctionDef)))
        {
            return p->_symbols[name]->appendAssignedNode(assignedNode,
                                                         postExprSuffixes,
                                                         derefCount, round);
        }
    }
    // We should never come here
    debugerr("Could not find variable with name \"" << name << "\" in scope!");
    return false;
}


void ASTScope::addSymbol(const QString &name, ASTSymbolType type,
						 ASTNode *node)
{
	if (_symbols.contains(name)) {
		bool warn = true;
		// See if we found a definition for a declaration
		switch (_symbols[name]->type()) {
		case stFunctionDecl:
			// No warning if definition follows declaration
			warn = (type != stFunctionDef);
			// no break
		case stFunctionDef:
			// Ignore declarations following prev. definition or declaration
			if (type == stFunctionDecl)
				return;
			// Special case: Ignore declarations like the following:
			// void foo() { /*...*/ }; typeof(foo) foo;
			if (type == stVariableDecl)
				return;
			break;

		case stVariableDecl:
            // Ignore declarations following prev. declaration
            if (type == stVariableDecl)
                return;
			// No warning if definition follows declaration
			warn = (type != stVariableDef);
			// No warning if function typedef'ed variable is followed by a
			// function definition
			warn = warn && (type != stFunctionDef);
			break;

		case stVariableDef:
			// Ignore declarations following prev. definition
			if (type == stVariableDecl)
				return;
			break;

		default:
			break;
		}

		if (warn)
			debugerr("Line " << (node ? node->start->line : 0) << ": scope already "
					 << "contains symbol \"" << name << "\" as "
					 << ASTSymbol::typeToString(_symbols[name]->type())
					 << ", defined at line "
					 << (_symbols[name]->astNode() ?
							 _symbols[name]->astNode()->start->line :
							 0)
					 << ", new type is " << ASTSymbol::typeToString(type)
					 );

		// Delete previous value
		delete _symbols[name];
	}

	_symbols.insert(name, new ASTSymbol(_ast, name, type, node));
}


void ASTScope::addCompoundType(const QString& name, ASTSymbolType type,
		struct ASTNode* node)
{
	if (_compoundTypes.contains(name)) {
		bool warn = true;
		// See if we found a definition for a declaration
		switch (_compoundTypes[name]->type()){
		case stEnumDecl:
			// No warning if definition follows declaration
			warn = (type != stEnumDef);
			// no break
		case stEnumDef:
			// Ignore declarations following prev. definition or declaration
			if (type == stEnumDecl)
				return;
			break;

		case stStructOrUnionDecl:
			// No warning if definition follows declaration
			warn = (type != stStructOrUnionDef);
			// no break
		case stStructOrUnionDef:
			// Ignore declarations following prev. definition or declaration
			if (type == stStructOrUnionDecl)
				return;
			break;

		default:
			break;
		}

		if (warn)
			debugerr("Line " << (node ? node->start->line : 0) << ": scope already "
					 << "contains tagged type \"" << qPrintable(name) << "\" as "
					 << ASTSymbol::typeToString(_compoundTypes[name]->type())
					 << ", defined at line "
					 << (_compoundTypes[name]->astNode() ?
							 _compoundTypes[name]->astNode()->start->line :
							 0)
					 << ", new type is " << ASTSymbol::typeToString(type)
					 );

		// Delete previous value
		delete _compoundTypes[name];
	}

	_compoundTypes.insert(name, new ASTSymbol(_ast, name, type, node));
}


void ASTScope::addTypedef(const QString& name, ASTSymbolType type,
		struct ASTNode* node)
{
	if (_typedefs.contains(name)) {
        debugerr("Line " << (node ? node->start->line : 0) << ": scope already "
                 << "contains tagged type \"" << qPrintable(name) << "\" as "
                 << qPrintable(ASTSymbol::typeToString(_typedefs[name]->type()))
                 << ", defined at line "
                 << (_typedefs[name]->astNode() ?
                         _typedefs[name]->astNode()->start->line :
                         0)
                 << ", new type is " << qPrintable(ASTSymbol::typeToString(type))
                 );

		// Delete previous value
		delete _typedefs[name];
	}

	_typedefs.insert(name, new ASTSymbol(_ast, name, type, node));
}


ASTSymbol* ASTScope::find(const QString& name, int searchSymbols) const
{
#   undef DEBUG_SCOPE_FIND
//#   define DEBUG_SCOPE_FIND 1

	if (searchSymbols & ssSymbols)
		for (const ASTScope* p = this; p; p = p->_parent) {
#           ifdef DEBUG_SCOPE_FIND
            debugmsg(
                    QString("Searching for symbol \"%1\" in %2 within scope %3 opened at %4:%5")
                    .arg(name)
                    .arg("symbols")
                    .arg((quint64)p, 0, 16)
                    .arg(p->astNode() ? p->astNode()->start->line : 0)
                    .arg(p->astNode() ? p->astNode()->start->charPosition : 0));
#           endif

            if (p->_symbols.contains(name)) {
#               ifdef DEBUG_SCOPE_FIND
                debugmsg(
                        QString("Found symbol \"%1\" in %2 within scope %3 opened at %4:%5")
                            .arg(name)
                            .arg("symbols")
                            .arg((quint64)p, 0, 16)
                            .arg(p->astNode() ? p->astNode()->start->line : 0)
                            .arg(p->astNode() ? p->astNode()->start->charPosition : 0));
#               endif

				return p->_symbols[name];
            }
		}
	if (searchSymbols & ssTypedefs)
		for (const ASTScope* p = this; p; p = p->_parent) {
#           ifdef DEBUG_SCOPE_FIND
            debugmsg(
                    QString("Searching for symbol \"%1\" in %2 within scope %3 opened at %4:%5")
                    .arg(name)
                    .arg("typedefs")
                    .arg((quint64)p, 0, 16)
                    .arg(p->astNode() ? p->astNode()->start->line : 0)
                    .arg(p->astNode() ? p->astNode()->start->charPosition : 0));
#           endif

            if (p->_typedefs.contains(name)) {
#               ifdef DEBUG_SCOPE_FIND
                debugmsg(
                        QString("Found symbol \"%1\" in %2 within scope %3 opened at %4:%5")
                            .arg(name)
                            .arg("typedefs")
                            .arg((quint64)p, 0, 16)
                            .arg(p->astNode() ? p->astNode()->start->line : 0)
                            .arg(p->astNode() ? p->astNode()->start->charPosition : 0));
#               endif

				return p->_typedefs[name];
            }
		}
	if (searchSymbols & ssCompoundTypes)
		for (const ASTScope* p = this; p; p = p->_parent) {
#           ifdef DEBUG_SCOPE_FIND
            debugmsg(
                    QString("Searching for symbol \"%1\" in %2 within scope %3 opened at %4:%5")
                    .arg(name)
                    .arg("compound types")
                    .arg((quint64)p, 0, 16)
                    .arg(p->astNode() ? p->astNode()->start->line : 0)
                    .arg(p->astNode() ? p->astNode()->start->charPosition : 0));
#           endif

			if (p->_compoundTypes.contains(name)) {
#               ifdef DEBUG_SCOPE_FIND
                debugmsg(
                        QString("Found symbol \"%1\" in %2 within scope %3 opened at %4:%5")
                            .arg(name)
                            .arg("compound types")
                            .arg((quint64)p, 0, 16)
                            .arg(p->astNode() ? p->astNode()->start->line : 0)
                            .arg(p->astNode() ? p->astNode()->start->charPosition : 0));
#               endif

				return p->_compoundTypes[name];
			}
		}

	return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ASTScopeManager::ASTScopeManager(const AbstractSyntaxTree *ast)
	: _currentScope(0), _ast(ast)
{
}


ASTScopeManager::~ASTScopeManager()
{
	clear();
}


void ASTScopeManager::clear()
{
	// Delete all scopes
	for (int i = 0; i < _scopes.size(); ++i)
		delete _scopes[i];
	_scopes.clear();
	_currentScope = 0;
}


void ASTScopeManager::pushScope(struct ASTNode* astNode)
{
	// Create new scope object
	_currentScope = new ASTScope(_ast, astNode, _currentScope);
	_scopes.append(_currentScope);
	if (astNode)
	    astNode->childrenScope = _currentScope;
}


void ASTScopeManager::popScope()
{
	if (_currentScope)
		_currentScope = _currentScope->parent();
	else
		debugerr("Called " << __PRETTY_FUNCTION__ << " on an empty scope stack");
}


void ASTScopeManager::addSymbol(const QString& name, ASTSymbolType type,
		struct ASTNode* node)
{
	addSymbol(name, type, node, _currentScope);
}


void ASTScopeManager::addSymbol(const QString& name, ASTSymbolType type,
		struct ASTNode* node, ASTScope* scope)
{
	if (!scope) {
		QString msg = (scope == _currentScope) ?
				"No scope has been initialized yet" :
				"Gone beyond last scope";
	    if (node)
	        debugerr(msg << ", ignoring symbol \"" << name << "\" in line "
	        		<< node->start->line);
	    else
            debugerr(msg << ", ignoring symbol \"" << name << "\"");
		return;
	}

	// If this is a struct/union definition, add it to the top-most scope that
	// does not belong to a struct or union
	if (type == stStructOrUnionDef || type == stEnumDef || type == stEnumerator) {
        while (scope->parent() && scope->astNode() &&
                scope->astNode()->type == nt_struct_or_union_specifier)
            scope = scope->parent();
	}

	scope->add(name, type, node);
}
