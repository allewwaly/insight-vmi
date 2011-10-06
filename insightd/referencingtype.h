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

class SymFactory;

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
    const BaseType* refType() const;

    /**
     * Getter for the directly referenced type
     * @return the type this referencing type directly points to
     */
    BaseType* refType();

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
     * Adds \a id to the list of alternative referencing type IDs.
     * @param id the ID to add
     * \sa hasAltRefTypes(), altRefType(), refTypeId()
     */
    void addAltRefTypeId(int id);

    /**
     * @return \c true if at least one alternative referencing type ID exists,
     *   \c false otherwise
     */
    bool hasAltRefTypes() const;

    /**
     * @return the number of alternative referencing type IDs
     */
    int altRefTypeCount() const;

    /**
     * When this symbol has alternative referencing type IDs and \a index is -1,
     * this function returns the most useful type, e. g., a struct or union, if
     * possible, or an otherwise typed pointer. If multiple such potential
     * candidates exist, the first one that is found is returned.
     *
     * If \a index is >= 0, then the alterantive type with that list index is
     * returned, if available, otherwise \c null is returned.
     *
     * @param index the list index of the alterative type, or -1 for a "best
     *   guess"
     * @return the alternative referencing type if found, \c null otherwise
     */
    BaseType* altRefType(int index = -1);

    /**
     * Overloaded method.
     * \sa altRefType()
     */
    const BaseType* altRefType(int index = -1) const;

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

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual SymFactory* fac() = 0;

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual const SymFactory* fac() const = 0;

    int _refTypeId;            ///< holds ID of the type this object is referencing    
    QList<int> _altRefTypeIds; ///< a list of alternative types

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

    template<class T>
    T* altRefTypeTempl(int index = -1) const;

};


inline int ReferencingType::refTypeId() const
{
    return _refTypeId;
}


inline void ReferencingType::setRefTypeId(int id)
{
    _refTypeId = id;
}


inline void ReferencingType::addAltRefTypeId(int id)
{
    if (!_altRefTypeIds.contains(id))
        _altRefTypeIds.append(id);
}


inline bool ReferencingType::hasAltRefTypes() const
{
    return !_altRefTypeIds.isEmpty();
}


inline int ReferencingType::altRefTypeCount() const
{
    return _altRefTypeIds.size();
}

#endif /* REFERENCINGTYPE_H_ */


#include "symfactory.h"

#if !defined(REFERENCINGTYPE_H_INLINE) && defined(SYMFACTORY_DEFINED)
#define REFERENCINGTYPE_H_INLINE

inline const BaseType* ReferencingType::refType() const
{
    return fac() && _refTypeId ? fac()->findBaseTypeById(_refTypeId) : 0;
}


inline BaseType* ReferencingType::refType()
{
    return fac() && _refTypeId ? fac()->findBaseTypeById(_refTypeId) : 0;
}


inline BaseType* ReferencingType::altRefType(int index)
{
    return altRefTypeTempl<BaseType>(index);
}


inline const BaseType* ReferencingType::altRefType(int index) const
{
    return altRefTypeTempl<const BaseType>(index);
}


template<class T>
inline T* ReferencingType::altRefTypeTempl(int index) const
{
    if (_altRefTypeIds.isEmpty() || index >= _altRefTypeIds.size() || !fac())
        return 0;
    if (index >= 0)
        return fac()->findBaseTypeById(_altRefTypeIds[index]);

    // No index given, find the most usable type

    // If we have only one alternative, the job is easy
    if (_altRefTypeIds.size() == 1)
        return fac()->findBaseTypeById(_altRefTypeIds.first());
    else {
        RealType useType = rtUndefined;
        T *t, *useBt = 0;
        for (int i = 0; i < _altRefTypeIds.size(); ++i) {
            if (!(t = fac()->findBaseTypeById(_altRefTypeIds[i])))
                continue;
            RealType curType = t->dereferencedType();
            // Init variables
            if (!useBt) {
                useBt = t;
                useType = curType;
            }
            // Compare types
            else {
                // Prefer structs/unions, followed by function pointers,
                // followed by pointers
                if ((curType & StructOrUnion) ||
                    (((useType & (NumericTypes|rtPointer)) && curType == rtFuncPointer)) ||
                    ((useType & NumericTypes) && curType == rtPointer))
                {
                    useType = curType;
                    useBt = t;
                }
            }
            if (useType & StructOrUnion)
                break;
        }

        return useBt;
    }
}

#endif /* REFERENCINGTYPE_H_INLINE */
