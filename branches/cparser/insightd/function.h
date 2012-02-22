#ifndef FUNCTION_H
#define FUNCTION_H

#include "funcpointer.h"

/**
 * This class represents a function definition.
 *
 * \sa FuncPointer, FuncParam
 */
class Function : public FuncPointer
{
public:
    /**
     * Constructor
     * @param factory the factory that created this symbol
     */
    explicit Function(SymFactory* factory);

    /**
     * Constructor
     * @param factory the factory that created this symbol
     * @param info the type information to construct this type from
     */
    Function(SymFactory* factory, const TypeInfo& info);

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
	 * @return the low program counter
	 */
	inline size_t pcLow() const
	{
		return _pcLow;
	}

	/**
	 * @return the high program counter
	 */
	inline size_t pcHigh() const
	{
		return _pcHigh;
	}

	/**
	 * @return was this function inlined
	 */
	inline bool inlined() const
	{
		return _inlined;
	}

    /**
     * This gives a pretty name of that type which may involve referencing
     * types.
     * @return the pretty name of that type, e.g. "const int[16]" or "const char *"
     */
    virtual QString prettyName() const;

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
    virtual void readFrom(KernelSymbolStream &in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(KernelSymbolStream &out) const;

private:
    bool _inlined;  ///< Is this an inlined function?
    size_t _pcLow;  ///< Low program counter
    size_t _pcHigh; ///< High program counter
};

#endif // FUNCTION_H
