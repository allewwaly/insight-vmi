
#include <insight/typeruleenginecontextprovider.h>
#include <insight/typeruleengine.h>


TypeRuleEngineContextProvider::TypeRuleEngineContextProvider(KernelSymbols *symbols)
    : _ctx(TypeRuleEngine::createContext(symbols))
{
}


TypeRuleEngineContextProvider::~TypeRuleEngineContextProvider()
{
    TypeRuleEngine::deleteContext(_ctx);
}
