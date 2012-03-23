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
	stTypedef            = (1 <<  0),
	stVariableDecl       = (1 <<  1),
	stVariableDef        = (1 <<  2),
	stStructOrUnionDecl  = (1 <<  3),
	stStructOrUnionDef   = (1 <<  4),
	stStructMember       = (1 <<  5),
	stFunctionDecl       = (1 <<  6),
	stFunctionDef        = (1 <<  7),
	stFunctionParam      = (1 <<  8),
	stEnumDecl           = (1 <<  9),
	stEnumDef            = (1 << 10),
	stEnumerator         = (1 << 11)
};

#endif /* ASTSYMBOLTYPES_H_ */
