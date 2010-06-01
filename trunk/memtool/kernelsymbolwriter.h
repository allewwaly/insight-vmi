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

class KernelSymbolWriter: public LongOperation
{
public:
    KernelSymbolWriter(QIODevice* to, SymFactory* factory);
    void write();

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();

private:
    QIODevice* _to;
    SymFactory* _factory;
};

#endif /* KERNELSYMBOLWRITER_H_ */
