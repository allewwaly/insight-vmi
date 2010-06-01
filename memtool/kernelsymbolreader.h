/*
 * kernelsymbolreader.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLREADER_H_
#define KERNELSYMBOLREADER_H_

#include "longoperation.h"

// forward declarations
class QIODevice;
class SymFactory;


class KernelSymbolReader: public LongOperation
{
public:
    KernelSymbolReader(QIODevice* from, SymFactory* factory);
    void read();

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();

private:
    QIODevice* _from;
    SymFactory* _factory;
};

#endif /* KERNELSYMBOLREADER_H_ */
