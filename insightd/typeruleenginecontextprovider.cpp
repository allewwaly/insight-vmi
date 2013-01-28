
#include "typeruleenginecontextprovider.h"
#include "typeruleengine.h"


TypeRuleEngineContextProvider::TypeRuleEngineContextProvider(KernelSymbols *symbols)
    : _ctx(TypeRuleEngine::createContext(symbols))
{
}


TypeRuleEngineContextProvider::~TypeRuleEngineContextProvider()
{
    TypeRuleEngine::deleteContext(_ctx);
}
