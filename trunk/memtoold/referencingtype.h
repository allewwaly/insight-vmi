/*
 * referencingtype.h
 *
 *  Created on: 01.04.2010
 *      Author: chrschn
 */

#ifndef REFERENCINGTYPE_H_
#define REFERENCINGTYPE_H_

#include "typeinfo.h"
#include "instance.h"

class BaseType;

class ReferencingType
{
public:
    /**
     * Constructor
     */
    ReferencingType();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    ReferencingType(const TypeInfo& info);

    /**
     * Destructor, just required to make this type polymorphical
     */
    virtual ~ReferencingType();

    /**
     * Getter for the directly referenced type
     * @return the type this referencing type directly points to
     */
    const BaseType* refType() const;

    /**
     * Follows all referencing types' references until a non-referencing
     * type was found.
     * @return the type this and all chained referencing types point to
     */
    const BaseType* refTypeDeep() const;

    /**
     * Set the base type this pointer points to
     * @param type new pointed type
     */
    void setRefType(const BaseType* type);

    /**
     * @return ID of the type this object is referencing
     */
    int refTypeId() const;

    /**
     * Sets the new ID of the type this object is referencing
     * @param id new ID
     */
    void setRefTypeId(int id);

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

protected:
    /**
     * Creates an Instance object of a StructuredMember object.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param parent the parent instance (if it's not a variable instance)
     * @param resolveTypes bitwise or'ed BaseType::RealType's that should
     * be resolved
     * @param derefCount pointer to a counter variable for how many types have
     * been followed to create the instance
     * @return an Instance object for this member
     */
    Instance createRefInstance(size_t address, VirtualMemory* vmem,
    		const QString& name, const Instance* parent,
    		int resolveTypes, int* derefCount = 0) const;

    /**
     * Creates an Instance object of a StructuredMember object.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param parentNames the name components of the parent (if it's not a
     * variable instance)
     * @param resolveTypes bitwise or'ed BaseType::RealType's that should
     * be resolved
     * @param derefCount pointer to a counter variable for how many types have
     * been followed to create the instance
     * @return an Instance object for this member
     */
    Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const ConstPStringList& parentNames,
            int resolveTypes, int* derefCount = 0) const;

    /**
     * Creates an Instance object of a Variable object.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param id the id of the variable
     * @param resolveTypes bitwise or'ed BaseType::RealType's that should
     * be resolved
     * @param derefCount pointer to a counter variable for how many types have
     * been followed to create the instance
     * @return an Instance object for this member
     */
    Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, int id, int resolveTypes,
            int* derefCount = 0) const;

	const BaseType *_refType;  ///< holds the type this object is referencing
    int _refTypeId;            ///< holds ID of the type this object is referencing

private:
    /**
     * Creates an Instance object of the \e referenced type.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param parent the parent instance, if any
     * @param parentNames the name components of the parent (if it's not a
     * variable instance)
     * @param id the id of the instance (if it is a variable instance)
     * @param resolveTypes bitwise or'ed BaseType::RealType's that should
     * be resolved
     * @param derefCount pointer to a counter variable for how many types have
     * been followed to create the instance
     * @return an Instance object for this member
     */
    inline Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const Instance* parent,
            const ConstPStringList& parentNames, int id, int resolveTypes,
            int* derefCount) const;
};


#endif /* REFERENCINGTYPE_H_ */
