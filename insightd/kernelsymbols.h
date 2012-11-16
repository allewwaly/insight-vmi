/*
 * kernelsymbols.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLS_H_
#define KERNELSYMBOLS_H_

#include "symfactory.h"
#include "typeinfo.h"
#include "genericexception.h"
#include "memspecs.h"
#include "typeruleengine.h"

// Forward declaration
class QIODevice;

/**
 * This class represents one instance of kernel symbols and provides functions
 * to parse, save and load the symbols from files.
 */
class KernelSymbols
{
public:
    /**
     * Constructor
     */
	KernelSymbols();

	/**
	 * Destructor
	 */
	~KernelSymbols();

	/**
	 * Parses the kernel debugging symbols from the kernel source tree
	 * \a kernelSrc. The Linux kernel \c vmlinux with debugging symbols and the
	 * \c System.map file must be directly located in \a kernelSrc.
	 * @param kernelSrc path to configured and built kernel source
	 */
	void parseSymbols(const QString& kernelSrc);

	/**
	 * Loads the kernel debugging symbols from the given device.
	 * @param from device to load the symbols from
	 */
    void loadSymbols(QIODevice* from);

    /**
     * Loads the kernel debugging symbols from the file given by \a fileName.
     * @param fileName the name of the file to load the symbols from
     */
    void loadSymbols(const QString& fileName);

    /**
     * Saves the kernel debugging symbols to the given device.
     * @param to device to save the symbols to
     */
    void saveSymbols(QIODevice* to);

    /**
     * Saves the kernel debugging symbols to the file given by \a fileName.
     * @param fileName the name of the file to save the symbols to
     */
    void saveSymbols(const QString& fileName);

    /**
     * Loads type knowledge rules from \a fileName into the rules engine.
     * @param fileName file to load
     * @param forceRead force to re-read files that have already been read
     */
    void loadRules(const QString& fileName, bool forceRead = false);

    /**
     * Loads type knowledge rules from all files passed in \a fileNames into
     * the rules engine.
     * @param fileNames list of files to load
     * @param forceRead force to re-read files that have already been read
     */
    void loadRules(const QStringList& fileNames, bool forceRead = false);

    /**
     * Deletes all rules within the engine
     */
    void flushRules();

    /**
     * Checks if there are rules available in the ruleEngine().
     */
    bool hasRules() const;

    /**
     * @return the symbol factory that holds all kernel symbols
     */
    SymFactory& factory();

    /**
     * @return the symbol factory that holds all kernel symbols (const version)
     */
    const SymFactory& factory() const;

    /**
     * @return the memory specifications of the kernel symbols
     */
    const MemSpecs& memSpecs() const;

    /**
     * Returns the rules engine.
     */
    TypeRuleEngine& ruleEngine();

    /**
     * Returns the rules engine (const version).
     */
    const TypeRuleEngine& ruleEngine() const;

    /**
     * @return file name of the loaded kernel symbols, if any
     */
    const QString& fileName() const;

    /**
     * @return \c true if kernel symbols have been loaded or parsed, \c false
     * otherwise
     */
    bool symbolsAvailable() const;

private:
	void checkRules();

    MemSpecs _memSpecs;
    SymFactory _factory;
    TypeRuleEngine _ruleEngine;
    QString _fileName;
};


inline bool KernelSymbols::hasRules() const
{
    return _ruleEngine.count() > 0;
}


inline SymFactory& KernelSymbols::factory()
{
    return _factory;
}


inline const SymFactory& KernelSymbols::factory() const
{
    return _factory;
}


inline const MemSpecs&  KernelSymbols::memSpecs() const
{
    return _memSpecs;
}


inline TypeRuleEngine &KernelSymbols::ruleEngine()
{
    return _ruleEngine;
}


inline const TypeRuleEngine &KernelSymbols::ruleEngine() const
{
    return _ruleEngine;
}


inline const QString& KernelSymbols::fileName() const
{
    return _fileName;
}


inline bool KernelSymbols::symbolsAvailable() const
{
    return !_factory.types().isEmpty();
}

#endif /* KERNELSYMBOLS_H_ */
