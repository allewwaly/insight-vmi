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
#include <QHash>
#include "genericexception.h"

// Forward declaration
class QIODevice;


class KernelSymbols
{
public:
	KernelSymbols();

	~KernelSymbols();

	void parseSymbols(QIODevice* from);
	void parseSymbols(const QString& fileName);

    void loadSymbols(QIODevice* from);
    void loadSymbols(const QString& fileName);

    void saveSymbols(QIODevice* to);
    void saveSymbols(const QString& fileName);

    const SymFactory& factory() const;

private:
	SymFactory _factory;
};

#endif /* KERNELSYMBOLS_H_ */
