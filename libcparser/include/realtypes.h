/*
 * realtypes.h
 *
 *  Created on: Mar 24, 2011
 *      Author: vogls
 */

#ifndef REALTYPES_H_
#define REALTYPES_H_

#include <QString>

/*
 * Changes made to this enum should also be made to the TypeManager constructor.
 */
enum RealType
{
	rtUndefined   = 0,
	rtInt8        = (1 <<  0),
	rtUInt8       = (1 <<  1),
	rtBool8       = (1 <<  2),
	rtInt16       = (1 <<  3),
	rtUInt16      = (1 <<  4),
	rtBool16      = (1 <<  5),
	rtInt32       = (1 <<  6),
	rtUInt32      = (1 <<  7),
	rtBool32      = (1 <<  8),
	rtInt64       = (1 <<  9),
	rtUInt64      = (1 << 10),
	rtBool64      = (1 << 11),
	rtFloat       = (1 << 12),
	rtDouble      = (1 << 13),
	rtPointer     = (1 << 14),
	rtArray       = (1 << 15),
	rtEnum        = (1 << 16),
	rtStruct      = (1 << 17),
	rtUnion       = (1 << 18),
	rtConst       = (1 << 19),
	rtVolatile    = (1 << 20),
	rtTypedef     = (1 << 21),
	rtFuncPointer = (1 << 22),
	rtVoid		  = (1 << 23),
	rtVaList	  = (1 << 24)
//	rtFuncCall    = (1 << 25)
};

static const int SignedIntegerTypes =
    rtInt8 |
    rtInt16 |
    rtInt32 |
    rtInt64;

static const int UnsignedIntegerTypes =
    rtUInt8 |
    rtUInt16 |
    rtUInt32 |
    rtUInt64;

static const int BooleanTypes =
    rtBool8 |
    rtBool16 |
    rtBool32 |
    rtBool64;

static const int IntegerTypes =
    BooleanTypes |
    SignedIntegerTypes |
    UnsignedIntegerTypes |
    rtEnum;

static const int FloatingTypes =
    rtFloat |
    rtDouble;

static const int NumericTypes =
    IntegerTypes |
    FloatingTypes;

QString realTypeToStr(RealType type);

#endif /* REALTYPES_H_ */
