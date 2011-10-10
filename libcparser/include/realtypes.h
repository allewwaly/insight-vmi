/*
 * realtypes.h
 *
 *  Created on: Mar 24, 2011
 *      Author: vogls
 */

#ifndef REALTYPES_H_
#define REALTYPES_H_

#include <QString>

/**
 * The actual type that a BaseType object represents.
 * Changes made to this enum should also be made to the TypeManager constructor.
 */
enum RealType
{
	rtUndefined   = 0,          //!< rtUndefined
	rtInt8        = (1 <<  0),  //!< rtInt8
	rtUInt8       = (1 <<  1),  //!< rtUInt8
	rtBool8       = (1 <<  2),  //!< rtBool8
	rtInt16       = (1 <<  3),  //!< rtInt16
	rtUInt16      = (1 <<  4),  //!< rtUInt16
	rtBool16      = (1 <<  5),  //!< rtBool16
	rtInt32       = (1 <<  6),  //!< rtInt32
	rtUInt32      = (1 <<  7),  //!< rtUInt32
	rtBool32      = (1 <<  8),  //!< rtBool32
	rtInt64       = (1 <<  9),  //!< rtInt64
	rtUInt64      = (1 << 10),  //!< rtUInt64
	rtBool64      = (1 << 11),  //!< rtBool64
	rtFloat       = (1 << 12),  //!< rtFloat
	rtDouble      = (1 << 13),  //!< rtDouble
	rtPointer     = (1 << 14),  //!< rtPointer
	rtArray       = (1 << 15),  //!< rtArray
	rtEnum        = (1 << 16),  //!< rtEnum
	rtStruct      = (1 << 17),  //!< rtStruct
	rtUnion       = (1 << 18),  //!< rtUnion
	rtConst       = (1 << 19),  //!< rtConst
	rtVolatile    = (1 << 20),  //!< rtVolatile
	rtTypedef     = (1 << 21),  //!< rtTypedef
	rtFuncPointer = (1 << 22),  //!< rtFuncPointer
	rtFunction    = (1 << 23),  //!< rtFunction
	rtVoid		  = (1 << 24),  //!< rtVoid
	rtVaList	  = (1 << 25)   //!< rtVaList
//	rtFuncCall    = (1 << 26)
};

/// Bitmask with all signed integer-based RealType's
static const int SignedIntegerTypes =
    rtInt8 |
    rtInt16 |
    rtInt32 |
    rtInt64;

/// Bitmask with all unsigned integer-based RealType's
static const int UnsignedIntegerTypes =
    rtUInt8 |
    rtUInt16 |
    rtUInt32 |
    rtUInt64;

/// Bitmask with all boolean-based RealType's
static const int BooleanTypes =
    rtBool8 |
    rtBool16 |
    rtBool32 |
    rtBool64;

/// Bitmask with all integer-based RealType's
static const int IntegerTypes =
    BooleanTypes |
    SignedIntegerTypes |
    UnsignedIntegerTypes |
    rtEnum;

/// Bitmask with all floating-point RealType's
static const int FloatingTypes =
    rtFloat |
    rtDouble;

/// Bitmask with all numeric (singed, unsinged, boolean, floating) RealType's
static const int NumericTypes =
    IntegerTypes |
    FloatingTypes;

/// Bitmask with function types
static const int FunctionTypes =
    rtFuncPointer |
    rtFunction;

/// These types need further resolution
static const int ReferencingTypes =
    FunctionTypes |
    rtPointer     |
    rtArray       |
    rtConst       |
    rtVolatile    |
    rtTypedef     |
    rtStruct      |
    rtUnion;

/// These types cannot be resolved anymore
static const int ElementaryTypes =
    NumericTypes |
    rtFuncPointer;

/// Structured types
static const int StructOrUnion =
    rtStruct |
    rtUnion;

QString realTypeToStr(RealType type);

#endif /* REALTYPES_H_ */
