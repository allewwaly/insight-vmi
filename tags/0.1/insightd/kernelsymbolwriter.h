/*
 * kernelsymbolwriter.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLWRITER_H_
#define KERNELSYMBOLWRITER_H_

#include "longoperation.h"

// forward declarations
class QIODevice;
class SymFactory;
struct MemSpecs;

/**
 * This class writes kernel symbols in a self-defined, compact format to a file
 * or any other QIODevice.
 */
class KernelSymbolWriter: public LongOperation
{
public:
    /**
     * Constructor
     * @param from destination to write the debugging symbols to
     * @param factory the SymFactory whose symbols shall be written
     * @param specs the MemSpecs whose data shall be written
     */
    KernelSymbolWriter(QIODevice* to, SymFactory* factory, MemSpecs* specs);

    /**
     * Starts the writing process.
     */
    void write();

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();

private:
    QIODevice* _to;
    SymFactory* _factory;
    MemSpecs* _specs;
};

#endif /* KERNELSYMBOLWRITER_H_ */
