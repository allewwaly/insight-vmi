/*
 * array.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef FUNCPOINTER_H_
#define FUNCPOINTER_H_

#include "refbasetype.h"
#include "funcparam.h"

class FuncPointer: public RefBaseType
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    explicit FuncPointer(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    FuncPointer(SymFactory* factory, const TypeInfo& info);

    /**
     * Destructor
     */
    virtual ~FuncPointer();

    /**
     * Create a hash of that type based on BaseType::hash(), srcLine() and the
     * name and hash() of all members.
     * @param isValid indicates if the hash is valid, for example, if all
     * referencing types could be resolved
     * @return a hash value of this type
     */
    virtual uint hash(bool* isValid = 0) const;

	/**
	 @return the actual type of that polimorphic variable
	 */
	virtual RealType type() const;

    /**
     * @return the list of parameters of this struct or union
     */
    inline const ParamList& params() const
    {
        return _params;
    }

    /**
     * Adds a parameter to this struct or union. This transfers the ownership of
     * \a param to this object, so \a param will be freed when the destructor
     * of this struct/union is called.
     * @param param the parameter to add, transfers ownership
     */
    void addParam(FuncParam* param);

	/**
	 * Finds out if a parameter with name \a paramName exists.
	 * @param paramName name of the param
	 * @return \c true if that parameter exists, \a false otherwise
	 */
	bool paramExists(const QString& paramName) const;

	/**
	 * Searches for a parameter with the name \a paramName.
	 * @param paramName name of the param to search
	 * @return the param, if it exists, \c 0 otherwise
	 */
	FuncParam* findParam(const QString& paramName);

    /**
     * Searches for a parameter with the name \a paramName (const version)
     * @param paramName name of the parameter to search
     * @return the parameter, if it exists, \c 0 otherwise
     */
    const FuncParam* findParam(const QString& paramName) const;

    /**
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

    /**
     * This gives a pretty name of this function pointer with the given name
     * as variable or typedef of name \a name.
     * @param name symbol name to use in the declaration
     * @return the pretty name of this function pointer, e.g., "int (*name)()"
     */
    QString prettyName(QString name, const RefBaseType *from) const;

    /**
     * Returns a pretty-printed list of the function's parameters.
     * @return formated parameters
     */
    QString prettyParams() const;

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
    virtual void writeTo(KernelSymbolStream& out) const;

protected:

    ParamList _params;  ///< Holds all parameters
};


#endif /* FUNCPOINTER_H_ */
