/*
 * symbol.h
 *
 *  Created on: 05.04.2010
 *      Author: chrschn
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include "typeinfo.h"
#include "kernelsymbolstream.h"

// forward declaration
class SymFactory;

/// Where was this symbol original obtained from?
enum SymbolSource {
    ssKernel,   ///< symbol was obtained from the kernel itself
    ssModule    ///< symbol was obtained from some kernel module
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
    explicit Symbol(SymFactory* factory);

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
     * Returns the local ID of this type, as assigned by SymFactory.
     * \sa setId(), origId()
     */
    int id() const;

    /**
     * Sets the local ID for this type.
     * @param id new ID
     * \sa id(), origId()
     */
    void setId(int id);

    /**
     * Returns the original ID of this type, as given by objdump output.
     * \sa setOrigId(), id()
     */
    int origId() const;

    /**
     * Sets the original ID for this type.
     * @param id new ID
     * \sa origId(), id()
     */
    void setOrigId(int id);

    /**
     * Returns the index of the kernel or module file this symbol was read from.
     * \sa setOrigFileIndex(), origFileName()
     */
    int origFileIndex() const;

    /**
     * Sets the the index of the kernel or module file this symbol was read from.
     * @param index new index
     * \sa origFileIndex()
     */
    void setOrigFileIndex(int index);

    /**
     * Returns the index of the kernel or module file this symbol was read from.
     * \sa origFileIndex()
     */
    const QString& origFileName() const;

    /**
     * Returns whether this symbol was obtained from the kernel or some module.
     */
    SymbolSource symbolSource() const;

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
    virtual void writeTo(KernelSymbolStream& out) const;

    /**
     * Returns the factory that created this symbol.
     */
    SymFactory* factory() const;

protected:
    /**
     * Sets the factory for this symbol.
     * @param factory the factory to set
     */
    void setFactory(SymFactory* factory);

    int _id;              ///< local ID of this type, assigned by SymFactory
    int _origId;          ///< original ID of this type, given by objdump
    int _origFileIndex;   ///< ID of this type, given by objdump
    QString _name;        ///< name of this type, e.g. "int"
    SymFactory* _factory; ///< the factory that created this symbol
    static const QString emptyString;
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


inline int Symbol::origId() const
{
    return _origId;
}


inline void Symbol::setOrigId(int id)
{
    _origId = id;
}


inline int Symbol::origFileIndex() const
{
    return _origFileIndex;
}


inline void Symbol::setOrigFileIndex(int index)
{
    _origFileIndex = index;
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
