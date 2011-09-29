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
#include "basetype.h"


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
     * Getter for the directly referenced type, const version
     * @return the type this referencing type directly points to
     */
    virtual const BaseType* refType() const = 0;

    /**
     * Getter for the directly referenced type
     * @return the type this referencing type directly points to
     */
    virtual BaseType* refType() = 0;

    /**
     * Follows all referencing types' references until a non-referencing
     * type was found.
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @return the type this and all chained referencing types point to
     */
    const BaseType* refTypeDeep(int resolveTypes) const;

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
     * When the refTypeId() of this object gets initially set, this ID will be
     * saved as origRefTypeId() so that further type adjustments can be
     * detected.
     * @return ID of the type this object was originally referencing
     */
    int origRefTypeId() const;

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
     * @param parentNames the name components of the parent (if it's not a
     * variable instance)
     * @param resolveTypes bitwise or'ed BaseType::RealType's that should
     * be resolved
     * @param derefCount pointer to a counter variable for how many types have
     * been followed to create the instance
     * @return an Instance object for this member
     */
    Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames,
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

    int _refTypeId;            ///< holds ID of the type this object is referencing    
    int _origRefTypeId;        ///< holds ID of the type this object was originally referencing

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
            const QString& name, const QStringList& parentNames, int id,
            int resolveTypes, int* derefCount) const;
};


inline int ReferencingType::refTypeId() const
{
    return _refTypeId;
}


inline int ReferencingType::origRefTypeId() const
{
    return _origRefTypeId;
}


inline void ReferencingType::setRefTypeId(int id)
{
    if (_origRefTypeId == 0)
        _origRefTypeId = (_refTypeId == 0) ? id : _refTypeId;
    _refTypeId = id;
}

#endif /* REFERENCINGTYPE_H_ */
