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
	 * Parses the kernel debugging symbols from the given device.
	 * @param from device to parse the symbols from
     * @param kernelSrc path to kernel source or header tree
     * @param systemMap path and file name to \c System.map file
	 */
	void parseSymbols(QIODevice* from, const QString& kernelSrc,
	        const QString systemMap);

	/**
	 * Parses the kernel debugging symbols from the file given by \a objdump
	 * and the memory specifications from \a kernelSrc and \a systemMap and
     * stores the result in _factory and _memSpecs.
	 * @param objdump the name of the objdump file to parse the symbols from
     * @param kernelSrc path to kernel source or header tree
     * @param systemMap path and file name to \c System.map file
	 */
	void parseSymbols(const QString& objdump, const QString& kernelSrc,
	        const QString systemMap);

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
     * @return the symbol factory that holds all kernel symbols
     */
    const SymFactory& factory() const;

    /**
     * @return the memory specifications of the kernel symbols
     */
    const MemSpecs& memSpecs() const;

    /**
     * @return file name of the loaded kernel symbols, if any
     */
    const QString& fileName() const;

private:
	MemSpecs _memSpecs;
    SymFactory _factory;
    QString _fileName;
};

#endif /* KERNELSYMBOLS_H_ */
