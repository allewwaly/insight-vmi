
#include "typeruleenginecontextprovider.h"
#include "typeruleengine.h"


TypeRuleEngineContextProvider::TypeRuleEngineContextProvider(const SymFactory *factory)
    : _ctx(TypeRuleEngine::createContext(factory))
{
}


TypeRuleEngineContextProvider::~TypeRuleEngineContextProvider()
{
    TypeRuleEngine::deleteContext(_ctx);
}
