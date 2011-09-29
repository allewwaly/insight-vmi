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
     * @param factory the factory that created this symbol
     */
    RefBaseType(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    RefBaseType(SymFactory* factory, const TypeInfo& info);

     /**
     * Create a hash of that type based on BaseType::hash() and refType()
     * @return a hash value of this type
     */
    virtual uint hash() const;

    /**
     * @return the size of this type in bytes
     */
    virtual uint size() const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset) const;

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(QDataStream& in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(QDataStream& out) const;

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
     * @param derefCount pointer to a counter variable for how many types have
     * been followed to create the instance
     * @return an Instance object for the dereferenced type
     * \sa BaseType::TypeResolution
     */
     virtual Instance toInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames,
            int resolveTypes = trLexical, int* derefCount = 0) const;
};


#endif /* REFBASETYPE_H_ */
