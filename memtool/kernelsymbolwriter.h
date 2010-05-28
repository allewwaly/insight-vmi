/*
 * kernelsymbolwriter.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLWRITER_H_
#define KERNELSYMBOLWRITER_H_

// forward declarations
class QIODevice;
class SymFactory;

class KernelSymbolWriter
{
public:
    KernelSymbolWriter(QIODevice* to, SymFactory* factory);
    void write();

private:
    QIODevice* _to;
    SymFactory* _factory;
};

#endif /* KERNELSYMBOLWRITER_H_ */
