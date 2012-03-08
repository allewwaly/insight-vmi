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
class ASTExpression;

class ReferencingType
{
public:
    struct AltRefType
    {
        explicit AltRefType(int id = -1, const ASTExpression* expr = 0)
            : id(id), expr(expr) {}
        int id;
        const ASTExpression* expr;

        /**
         * Creates an Instance of this alternative type.
         * @param vmem the VirtualMemory object to create the instance for
         * @param inst the Instance object to evaluate expr with
         * @param factory the factory to which this type belongs
         * @param name the name for this instance, i.e., variable or member name
         * @param parentNames the parent name components used to reach this
         * alternative type
         */
        Instance toInstance(VirtualMemory* vmem, const Instance* inst,
                            const SymFactory* factory, const QString &name,
                            const QStringList &parentNames) const;

        /**
         * Reads a serialized version of this object from \a in.
         * \sa writeTo()
         * @param in the data stream to read the data from, must be ready to read
         */
        void readFrom(KernelSymbolStream& in, SymFactory *factory);

        /**
         * Writes a serialized version of this object to \a out
         * \sa readFrom()
         * @param out the data stream to write the data to, must be ready to write
         */
        void writeTo(KernelSymbolStream &out) const;
    };

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
    BaseType* refTypeDeep(int resolveTypes);

    /**
     * Overloaded member function, const version.
     * \sa refTypeDeep()
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
    void addAltRefType(int id, const ASTExpression* expr);

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
     * Gives direct access to the alterantive referencing types
     * @return reference to the list of alternative types
     */
    QList<AltRefType>& altRefTypes();

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
    const AltRefType& altRefType(int index = -1) const;

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
    const BaseType* altRefBaseType(int index = -1) const;

    /**
     * This is an overloaded function.
     * \sa altRefBaseType()
     */
    BaseType* altRefBaseType(int index = -1);

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(KernelSymbolStream& in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(KernelSymbolStream &out) const;

    /**
     * Reads a serialized version of the alternative referencing types from
     * \a in.
     * @param in the data stream to read the data from, must be ready to read
     * @param factory the symbol factory this symbol belongs to
     */
    void readAltRefTypesFrom(KernelSymbolStream& in, SymFactory* factory);

    /**
     * Writes a serialized version of the alternative referencing types to
     * \a out
     * @param out the data stream to write the data to, must be ready to write
     */
    void writeAltRefTypesTo(KernelSymbolStream& out) const;

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
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param derefCount pointer to a counter variable for how many pointers
     * have been dereferenced to create the instance
     * @return an Instance object for this member
     */
    Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames,
            int resolveTypes, int maxPtrDeref = -1, int *derefCount = 0) const;

    /**
     * Creates an Instance object of a Variable object.
     * @param address the address of the instance within \a vmem
     * @param vmem the virtual memory object to read data from
     * @param name the name of this instance
     * @param id the id of the variable
     * @param resolveTypes bitwise or'ed BaseType::RealType's that should
     * be resolved
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @param derefCount pointer to a counter variable for how many pointers
     * have been dereferenced to create the instance
     * @return an Instance object for this member
     */
    Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, int id, int resolveTypes,
            int maxPtrDeref = -1, int* derefCount = 0) const;

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual SymFactory* fac() = 0;

    /**
     * Access function to the factory this symbol belongs to.
     */
    virtual const SymFactory* fac() const = 0;

    int _refTypeId;            ///< holds ID of the type this object is referencing    

    // Caching variables
    mutable BaseType* _refType;
    mutable BaseType* _refTypeDeep;
    mutable int _deepResolvedTypes;
    mutable quint32 _refTypeChangeClock;

    QList<AltRefType> _altRefTypes; ///< a list of alternative types
    static const AltRefType _emptyRefType;

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
     * @param derefCount pointer to a counter variable for how many pointers
     * have been dereferenced to create the instance
     * @return an Instance object for this member
     */
    inline Instance createRefInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames, int id,
            int resolveTypes, int maxPtrDeref, int* derefCount) const;

    template<class ref_t, class base_t>
    static base_t* refTypeTempl(ref_t *ref);

    template<class ref_t, class base_t, class ref_base_t>
    static base_t* refTypeDeepTempl(ref_t *ref, int resolveTypes);
};


inline int ReferencingType::refTypeId() const
{
    return _refTypeId;
}


inline void ReferencingType::setRefTypeId(int id)
{
    _refTypeId = id;
}


inline bool ReferencingType::hasAltRefTypes() const
{
    return !_altRefTypes.isEmpty();
}


inline int ReferencingType::altRefTypeCount() const
{
    return _altRefTypes.size();
}


inline QList<ReferencingType::AltRefType>& ReferencingType::altRefTypes()
{
    return _altRefTypes;
}

#endif /* REFERENCINGTYPE_H_ */


#include "symfactory.h"

#if !defined(REFERENCINGTYPE_H_INLINE) && defined(SYMFACTORY_DEFINED)
#define REFERENCINGTYPE_H_INLINE

#endif /* REFERENCINGTYPE_H_INLINE */
