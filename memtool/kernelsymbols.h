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
	 */
	void parseSymbols(QIODevice* from);

	/**
	 * Parses the kernel debugging symbols from the file given by \a fileName.
	 * @param fileName the name of the file to parse the symbols from
	 */
	void parseSymbols(const QString& fileName);

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

private:
	SymFactory _factory;
};

#endif /* KERNELSYMBOLS_H_ */
