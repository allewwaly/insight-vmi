#ifndef TYPERULEENGINECONTEXTPROVIDER_H
#define TYPERULEENGINECONTEXTPROVIDER_H

// Forward declaration
class TypeRuleEngineContext;
class KernelSymbols;

/**
 * Base class for multi-threaded usage of the TypeRuleEngine.
 *
 * If you need concurrent accesses to one single TypeRuleEngine, then each
 * QThread must inherit this class. It creates a unique context for the
 * engine using the static function TypeRuleEngine::createContext() and releases
 * it later with a call to TypeRuleEngine::deleteContext(). The engine will then
 * use the per-thread context when evaluating type rules.
 */
class TypeRuleEngineContextProvider
{
public:
    /**
     * Constructor
     * @param symbols the KernelSymbols used by the TypeRuleEngine
     */
    TypeRuleEngineContextProvider(KernelSymbols* symbols);

    /**
     * Destructor
     */
    virtual ~TypeRuleEngineContextProvider();

    inline TypeRuleEngineContext* typeRuleCtx() const { return _ctx; }

private:
    TypeRuleEngineContext* _ctx;
};

#endif // TYPERULEENGINECONTEXTPROVIDER_H
