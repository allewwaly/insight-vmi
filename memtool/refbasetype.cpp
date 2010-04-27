/*
 * refbasetype.cpp
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#include "refbasetype.h"
#include <assert.h>

RefBaseType::RefBaseType(const TypeInfo& info)
	: BaseType(info)
{
}


uint RefBaseType::hash(VisitedSet* visited) const
{
    uint ret = BaseType::hash(visited);
    if (_refType)
        ret ^= _refType->hash(visited);
    return ret;
}


//QString RefBaseType::name() const
//{
//    if (_refType)
//        return "const " + _refType->name();
//    else
//        return "const";
//}


uint RefBaseType::size() const
{
    if (!_size && _refType)
        return _refType->size();
    else
        return _size;
}


QString RefBaseType::toString(size_t offset) const
{
	assert(_refType != 0);
	return _refType->toString(offset);
}
