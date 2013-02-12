#ifndef FUNCTION_H
#define FUNCTION_H

#include "funcpointer.h"
#include "memorysection.h"

/**
 * This class represents a function definition.
 *
 * \sa FuncPointer, FuncParam
 */
class Function : public FuncPointer, public MemorySection
{
public:
    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     */
    explicit Function(KernelSymbols* symbols);

    /**
     * Constructor
     * @param symbols the kernel symbols this symbol belongs to
     * @param info the type information to construct this type from
     */
    Function(KernelSymbols* symbols, const TypeInfo& info);

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
     * Returns the size of this type in bytes
     */
    virtual inline uint size() const
    {
        if (_pcLow && _pcHigh)
            return _pcHigh - _pcLow;
        else
            return 0;
    }

	/**
	 * @return the low program counter
	 */
	inline quint64 pcLow() const
	{
		return _pcLow;
	}

	/**
	 * @return the high program counter
	 */
	inline quint64 pcHigh() const
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
     * \copydoc Symbol::prettyName()
     */
    virtual QString prettyName(const QString& varName = QString()) const;

    /**
     * @param mem the memory device to read the data from
     * @param offset the offset at which to read the value from memory
     * @return a string representation of this type
     */
    virtual QString toString(QIODevice* mem, size_t offset, const ColorPalette* col = 0) const;

    /**
     * @copydoc BaseType::toInstance()
     */
    virtual Instance toInstance(size_t address, VirtualMemory* vmem,
            const QString& name, const QStringList& parentNames,
            int resolveTypes = trLexical, int maxPtrDeref = -1,
            int* derefCount = 0) const;

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
    quint64 _pcLow;  ///< Low program counter
    quint64 _pcHigh; ///< High program counter
};

#endif // FUNCTION_H
