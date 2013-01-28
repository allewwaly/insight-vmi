/*
 * Variable.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

#include <sys/types.h>
#include <QVariant>
#include "symbol.h"
#include "referencingtype.h"
#include "sourceref.h"
#include <debug.h>

class TypeRuleEngine;

/**
 * This class represents a variable variable of a certain type.
 */
class Variable: public Symbol, public ReferencingType, public SourceRef
{
public:
    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     */
    explicit Variable(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    Variable(KernelSymbols* symbols, const TypeInfo& info);

    /**
     * Generic value function that will return the data as any type
     * @param mem the memory device to read the data from
     */
    template<class T>
    inline T value(VirtualMemory* mem) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        const BaseType* t = refType();
        assert(t != 0);
        return t->value<T>(mem, _offset);
    }

    /**
     * @param mem the memory device to read the data from
     * @return the value of this type as a variant
     */
    template<class T>
    inline QVariant toVariant(VirtualMemory* mem) const
    {
        // We put the implementation in the header to allow the compiler to
        // inline the code
        const BaseType* t = refType();
        assert(t != 0);
        return t->value<T>(mem, _offset);
    }

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

    /**
     * @param mem the memory device to read the data from
     * @return a string representation of this variable
     */
    QString toString(VirtualMemory* mem) const;

    /**
     * @return the offset of this variable in memory
     */
    quint64 offset() const;

	/**
	 * Sets the offset of this variable in memory
	 * @param offset new offset
	 */
	void setOffset(quint64 offset);

    /**
     * Creates an Instance object from this variable.
     * @param vmem the virtual memory object to read data from
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @param src knowledge sources to use for resolving the member
     * @param result provides the result of the knowledge source evaluation,
     * see TypeRuleEngine::MatchResult
     * @return an Instace object for this member
     * \sa KnowledgeSources
     */
    Instance toInstance(VirtualMemory* vmem, int resolveTypes =
            BaseType::trLexical, KnowledgeSources src = ksAll,
                        int *result = 0) const;

    /**
     * Retrieves an Instance for the alternative referencing type no. \a index
     * of this variable. You can check for the number alternative types with
     * altRefTypeCount().
     * @param vmem the VirtualMemory object to create the instance for
     * @param index index of the alternative type
     * @return a new Instance object for the selected alternative type, if it
     * exists, or an empty Instance otherwise
     */
    Instance altRefTypeInstance(VirtualMemory *vmem, int index) const;

	/**
	 * Reads a serialized version of this object from \a in.
	 * \sa writeTo()
	 * @param in the data stream to read the data from, must be ready to read
	 */
	virtual void readFrom(KernelSymbolStream &in);

	/**
	 * Writes a serialized version of this object to \a out
	 * \sa readFrom()
	 * @param out the data stream to write the data to, must be ready to write
	 */
	virtual void writeTo(KernelSymbolStream& out) const;

    /**
     * Sets the global rule engine used by all memory dumps.
     * @param engine engine to use
     */
    static void setRuleEngine(const TypeRuleEngine* engine);

    /**
     * Returns the global rule engine used by all memory dumps.
     */
    static const TypeRuleEngine* ruleEngine();


protected:
    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual SymFactory* fac();

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual const SymFactory* fac() const;

    quint64 _offset;
    static const TypeRuleEngine* _ruleEngine;
};

/**
* Operator for native usage of the Variable class for streams
* @param in data stream to read from
* @param var object to store the serialized data to
* @return the data stream \a in
*/
KernelSymbolStream& operator>>(KernelSymbolStream& in, Variable& var);


/**
* Operator for native usage of the Variable class for streams
* @param out data stream to write the serialized data to
* @param var object to serialize
* @return the data stream \a out
*/
KernelSymbolStream& operator<<(KernelSymbolStream& out, const Variable& var);


inline quint64 Variable::offset() const
{
    return _offset;
}


inline void Variable::setOffset(quint64 offset)
{
    _offset = offset;
}


inline void Variable::setRuleEngine(const TypeRuleEngine* engine)
{
    _ruleEngine = engine;
}


inline const TypeRuleEngine* Variable::ruleEngine()
{
    return _ruleEngine;
}


#endif /* VARIABLE_H_ */
