#include "typerule.h"
#include "typefilter.h"


TypeRule::TypeRule()
    : _filter(0), _osFilter(0), _actionType(atNone)
{
}


TypeRule::~TypeRule()
{
    if (_filter)
        delete _filter;
    // We don't create _osFilter, so don't delete it
}
