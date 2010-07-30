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

/**
 * Special IDs for special symbol types
 */
enum SpecialIds {
    siListHead = 0x7FFFFFFF      ///< kernel linked list (struct list_head)
};


/**
 * This class represents a generic debugging symbol read from the objdump output.
 */
class Symbol
{
public:
    /**
     * Constructor
     */
    Symbol();

	/**
	 * Constructor
     * @param info the type information to construct this type from
	 */
	Symbol(const TypeInfo& info);

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

protected:
    int _id;         ///< ID of this type, given by objdump
    QString _name;       ///< name of this type, e.g. "int"
};

#endif /* SYMBOL_H_ */
