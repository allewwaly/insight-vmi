#ifndef FUNCPARAM_H
#define FUNCPARAM_H

#include "basetype.h"
#include "referencingtype.h"
#include "sourceref.h"
#include "symbol.h"
#include "instance.h"

class FuncPointer;
class VirtualMemory;

/**
 * This class represents a parameter of a function or function pointer
 * definition.
 *
 * \sa FuncPointer, Function
 */
class FuncParam: public Symbol, public ReferencingType, public SourceRef
{
    friend class FuncPointer;

public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    explicit FuncParam(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    FuncParam(SymFactory* factory, const TypeInfo& info);

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

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
    virtual void writeTo(KernelSymbolStream &out) const;

    /**
     * @return the function pointer or function this parameter belongs to
     */
    FuncPointer* belongsTo() const;

    /**
     * Creates an Instance object from this parameter.
     * @param address the virtual memory address of this parameter
     * @param vmem the virtual memory object to read data from
     * @param parent the parent instance of this parameter
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @return an Instace object for this parameter
     */
    Instance toInstance(size_t address, VirtualMemory* vmem,
            const Instance *parent, int resolveTypes =
            BaseType::trLexical) const;

protected:
    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual SymFactory* fac();

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual const SymFactory* fac() const;

private:
    FuncPointer* _belongsTo;  ///< function this parameter belongs to
};


inline const SymFactory* FuncParam::fac() const
{
    return _factory;
}


inline SymFactory* FuncParam::fac()
{
    return _factory;
}


/**
 * Operator for native usage of the FuncParam class for streams
 * @param in data stream to read from
 * @param param object to store the serialized data to
 * @return the data stream \a in
 */
KernelSymbolStream& operator>>(KernelSymbolStream& in, FuncParam& param);


/**
 * Operator for native usage of the FuncParam class for streams
 * @param out data stream to write the serialized data to
 * @param param object to serialize
 * @return the data stream \a out
 */
KernelSymbolStream& operator<<(KernelSymbolStream& out, const FuncParam& param);


#endif // FUNCPARAM_H
