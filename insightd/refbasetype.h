/*
 * refbasetype.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef REFBASETYPE_H_
#define REFBASETYPE_H_

#include "basetype.h"
#include "referencingtype.h"
#include "instance.h"

class RefBaseType: public BaseType, public ReferencingType
{
public:
    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     */
    explicit RefBaseType(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    RefBaseType(KernelSymbols* symbols, const TypeInfo& info);

    /**
     * Create a hash of that type based on BaseType::hash() and refType()
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const;

    /**
     * @return the size of this type in bytes
     */
    virtual uint size() const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(VirtualMemory* mem, size_t offset,
                             const ColorPalette* col = 0) const;

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
     * Creates an Instance object of the referenced type. If that type again
     * is a RefBaseType, it is further dereferenced.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param parentNames the name components of the parent, i.e., nesting
     * struct
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param derefCount pointer to a counter variable for how many pointers
     * have been dereferenced to create the instance
     * @return an Instance object for the dereferenced type
     * \sa BaseType::TypeResolution
     */
     virtual Instance toInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames,
            int resolveTypes = trLexical, int maxPtrDeref = -1,
            int* derefCount = 0) const;

protected:
    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual SymFactory* fac();

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual const SymFactory* fac() const;

    mutable int _hashRefTypeId; ///< type ID used to generate the hash
};


inline uint RefBaseType::size() const
{
    if (!_size) {
        const BaseType* t = refType();
        return t ? t->size() : _size;
    }
    else
        return _size;
}

#endif /* REFBASETYPE_H_ */

