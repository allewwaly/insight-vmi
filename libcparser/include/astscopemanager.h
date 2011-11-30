/*
 * astscopemanager.h
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#ifndef ASTSCOPEMANAGER_H_
#define ASTSCOPEMANAGER_H_

#include <astsymbol.h>
#include <QHash>
#include <QList>
#include <QString>

struct ASTNode;
typedef QHash<QString, ASTSymbol*> ASTSymbolHash;


class ASTScope
{
	ASTSymbolHash _symbols;
	ASTSymbolHash _compoundTypes; // enum, structs, unions
	ASTSymbolHash _typedefs;
	struct ASTNode* _astNode;
	ASTScope* _parent;

	void addSymbol(const QString& name, ASTSymbolType type, struct ASTNode* node);
	void addCompoundType(const QString& name, ASTSymbolType type, struct ASTNode* node);
	void addTypedef(const QString& name, ASTSymbolType type, struct ASTNode* node);

public:
	enum SearchSymbols {
		ssSymbols       = (1 << 0),
		ssCompoundTypes = (1 << 1),
		ssTypedefs      = (1 << 2),
		ssAnyType       = ssCompoundTypes|ssTypedefs,
		ssAnySymbol     = ssSymbols|ssCompoundTypes|ssTypedefs
	};

	ASTScope(struct ASTNode* astNode, ASTScope* parent = 0)
		: _astNode(astNode), _parent(parent) {}

	~ASTScope();

	inline ASTSymbolHash& symbols() { return _symbols; }
	inline ASTSymbolHash& compoundTypes() { return _compoundTypes; }
	inline ASTSymbolHash& typedefs() { return _typedefs; }
	inline struct ASTNode* astNode() const { return _astNode; }
	inline ASTScope* parent() const { return _parent; }

	void add(const QString& name, ASTSymbolType type, struct ASTNode* node);
	bool varAssignment(const QString& name, const ASTNode *assignedNode,
					   int derefCount, int round);
	ASTSymbol* find(const QString& name, int searchSymbols = ssAnySymbol) const;
};


typedef QList<ASTScope*> ASTScopeList;
typedef QList<ASTScopeList> ASTScopeStack;


class ASTScopeManager
{
	ASTScopeList _scopes;
	ASTScope* _currentScope;

public:
	ASTScopeManager();
	virtual ~ASTScopeManager();

	void clear();
	void pushScope(struct ASTNode* astNode);
	void popScope();
	void addSymbol(const QString& name, ASTSymbolType type, struct ASTNode* node);
	void addSymbol(const QString& name, ASTSymbolType type, struct ASTNode* node,
			ASTScope* scope);
	inline ASTScope* currentScope() const { return _currentScope; }
};

#endif /* ASTSCOPEMANAGER_H_ */
