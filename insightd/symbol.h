/*
 * symbol.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "typeinfo.h"
#include <QDataStream>

// forward declaration
class SymFactory;

/**
 * Special IDs for special symbol types
 */
enum SpecialIds {
    siHListNode      = 0x7FFFFFFE,  ///< kernel hash chain list (<tt>struct hlist_node</tt>)
    siListHead       = 0x7FFFFFFF   ///< kernel linked list (<tt>struct list_head</tt>)
};


/**
 * This class represents a generic debugging symbol read from the objdump output.
 */
class Symbol
{
    friend class SymFactory;

public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    Symbol(SymFactory* factory);

	/**
	 * Constructor
	 * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
	 */
	Symbol(SymFactory* factory, const TypeInfo& info);

	/**
	 * Destructor
	 */
	virtual ~Symbol();

    /**
     * @return the name of that type, e.g. "int"
     */
    QString name() const;

    /**
     * Set the name of this symbol
     * @param name new name
     */
    void setName(const QString& name);

    /**
     * This gives a pretty name of that type which may involve referencing
     * types.
     * @return the pretty name of that type, e.g. "const int[16]" or "const char *"
     */
    virtual QString prettyName() const;

    /**
     * @return id ID of this type, as given by objdump output
     */
    int id() const;

    /**
     * Sets the ID of this type
     * @param id new ID
     */
    void setId(int id);

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
     * @return the factory that created this symbol
     */
    SymFactory* factory() const;

protected:
    /**
     * Sets the factory for this symbol
     * @param factory the factory to set
     */
    void setFactory(SymFactory* factory);

    int _id;         ///< ID of this type, given by objdump
    QString _name;       ///< name of this type, e.g. "int"
    SymFactory* _factory; ///< the factory that created this symbol
};


inline QString Symbol::name() const
{
    return _name;
}


inline void Symbol::setName(const QString& name)
{
    _name = name;
}


inline int Symbol::id() const
{
    return _id;
}


inline void Symbol::setId(int id)
{
    _id = id;
}


inline SymFactory* Symbol::factory() const
{
    return _factory;
}


inline void Symbol::setFactory(SymFactory* factory)
{
    _factory = factory;
}


#endif /* SYMBOL_H_ */
