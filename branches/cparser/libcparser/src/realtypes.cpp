/*
 * types.cpp
 *
 *  Created on: 26.04.2011
 *      Author: chrschn
 */

#include <realtypes.h>

QString realTypeToStr(RealType type)
{
    switch (type) {
    case rtUndefined:   return "Undefined";
    case rtInt8:        return "Int8";
    case rtUInt8:       return "UInt8";
    case rtBool8:	    return "Bool8";
    case rtInt16:	    return "Int16";
    case rtUInt16:	    return "UInt16";
    case rtBool16:	    return "Bool16";
    case rtInt32:	    return "Int32";
    case rtUInt32:	    return "UInt32";
    case rtBool32:	    return "Bool32";
    case rtInt64:	    return "Int64";
    case rtUInt64:	    return "UInt64";
    case rtBool64:	    return "Bool64";
    case rtFloat:	    return "Float";
    case rtDouble:	    return "Double";
    case rtPointer:	    return "Pointer";
    case rtArray:	    return "Array";
    case rtEnum:	    return "Enum";
    case rtStruct:	    return "Struct";
    case rtUnion:	    return "Union";
    case rtConst:	    return "Const";
    case rtVolatile:	return "Volatile";
    case rtTypedef:	    return "Typedef";
    case rtFuncPointer:	return "FuncPointer";
    case rtFunction:    return "Function";
    case rtVoid:	    return "Void";
    case rtVaList:	    return "VaList";
//    case rtFuncCall:	return "FuncCall";
    };

    return "Unknown";
};
