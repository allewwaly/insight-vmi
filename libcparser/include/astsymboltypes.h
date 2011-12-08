/*
 * astsymboltypes.h
 *
 *  Created on: 06.04.2011
 *      Author: chrschn
 */

#ifndef ASTSYMBOLTYPES_H_
#define ASTSYMBOLTYPES_H_

enum ASTSymbolType
{
	stNull = 0,
	stTypedef,
	stVariableDecl,
	stVariableDef,
	stStructOrUnionDecl,
	stStructOrUnionDef,
	stStructMember,
	stFunctionDecl,
	stFunctionDef,
	stFunctionParam,
	stEnumDecl,
	stEnumDef,
	stEnumerator
};

#endif /* ASTSYMBOLTYPES_H_ */
