/*
 * structmember.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef STRUCTUREDMEMBER_H_
#define STRUCTUREDMEMBER_H_

#include "basetype.h"
#include "referencingtype.h"
#include "sourceref.h"
#include "symbol.h"
#include "instance.h"

class Structured;
class VirtualMemory;


class StructuredMember: public Symbol, public ReferencingType, public SourceRef
{
    friend class Structured;

public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    explicit StructuredMember(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    StructuredMember(SymFactory* factory, const TypeInfo& info);

    /**
     * Returns the index of this member into its structure's member list.
     * @return index into member list
     */
    int index() const;

    /**
     * @return the offset of this member within a struct or union
     */
    size_t offset() const;

    /**
     * Sets a new value for the offset.
     * @param offset the new offset
     */
    void setOffset(size_t offset);

    /**
     * @return the bit size of this bit-field integer declaration
     */
    int bitSize() const;

    /**
     * Sets the bit size of this bit-field integer declaration.
     * @param size new bit size of bit-field integer declaration
     */
    void setBitSize(qint8 size);

    /**
     * @return the bit offset of this bit-field integer declaration
     */
    int bitOffset() const;

    /**
     * Sets the bit offset of this bit-field integer declaration.
     * @param offset new bit offset of bit-field integer declaration
     */
    void setBitOffset(qint8 offset);

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
    virtual void writeTo(KernelSymbolStream& out) const;

    /**
     * @return the Struct or Union this member belongs to
     */
    Structured* belongsTo() const;

    /**
     * Creates an Instance object from this struct member.
     * @param structAddress the virtual memory address of the containing (parent) struct
     * @param vmem the virtual memory object to read data from
     * @param parent the parent instance of this member
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
     * @param maxPtrDeref the maximum levels of pointers that should be
     * dereferenced
     * @return an Instace object for this member
     */
    Instance toInstance(size_t structAddress, VirtualMemory* vmem,
                        const Instance *parent,
                        int resolveTypes = BaseType::trLexical,
                        int maxPtrDeref = -1) const;

    /*
      * Function to save result of magic number investigation.
      * @returns true if object has already been seen
      */
    bool evaluateMagicNumberFoundNotConstant();
    
    /*
      * Function to save result of magic number investigation.
      * Should be called if constant integer type is found.
      * @returns true if object has already been seen
      */
    bool evaluateMagicNumberFoundInt(qint64 constant);
    
    /*
      * Function to save result of magic number investigation.
      * Should be called if constant string is found.
      * @returns true if object has already been seen
      */
    bool evaluateMagicNumberFoundString(const QString& constant);

    inline bool hasNotConstValue() const
    {
        return (_seenInEvaluateMagicNumber) ? !(_hasConstIntValue || _hasConstStringValue) : false;
    }
    
    inline bool hasConstantIntValue() const
    {
        return _hasConstIntValue;
    }
    inline bool hasConstantStringValue() const
    {
        return _hasConstStringValue;
    }
    inline bool hasStringValue() const
    {
        return _hasStringValue;
    }
    inline  const QList<qint64>& constantIntValue() const
    {
        return _constIntValue;
    }
    inline const QList<QString>& constantStringValue() const
    {
        return _constStringValue;
    }

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
	size_t _offset;          ///< the member's offset within the struct;
	Structured* _belongsTo;  ///< Struct or Union this member belongs to
    
    bool _seenInEvaluateMagicNumber;
    QList<qint64> _constIntValue;
    QList<QString> _constStringValue;
    bool _hasConstIntValue;
    bool _hasConstStringValue;
    bool _hasStringValue;
    qint8 _bitSize;
    qint8 _bitOffset;
};


inline const SymFactory* StructuredMember::fac() const
{
	return _factory;
}


inline SymFactory* StructuredMember::fac()
{
    return _factory;
}


inline size_t StructuredMember::offset() const
{
    return _offset;
}


inline void StructuredMember::setOffset(size_t offset)
{
    _offset = offset;
}


inline Structured* StructuredMember::belongsTo() const
{
    return _belongsTo;
}


inline int StructuredMember::bitSize() const
{
    return _bitSize;
}


inline void StructuredMember::setBitSize(qint8 size)
{
    _bitSize = size;
}


inline int StructuredMember::bitOffset() const
{
    return _bitOffset;
}


inline void StructuredMember::setBitOffset(qint8 offset)
{
    _bitOffset = offset;
}


/**
 * Operator for native usage of the StructuredMember class for streams
 * @param in data stream to read from
 * @param member object to store the serialized data to
 * @return the data stream \a in
 */
KernelSymbolStream& operator>>(KernelSymbolStream& in, StructuredMember& member);


/**
 * Operator for native usage of the StructuredMember class for streams
 * @param out data stream to write the serialized data to
 * @param member object to serialize
 * @return the data stream \a out
 */
KernelSymbolStream& operator<<(KernelSymbolStream& out, const StructuredMember& member);

#endif /* STRUCTUREDMEMBER_H_ */
