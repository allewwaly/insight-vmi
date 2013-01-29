/*
 * kernelsymbolreader.h
 *
 *  Created on: 28.05.2010
 *      Author: chrschn
 */

#ifndef KERNELSYMBOLREADER_H_
#define KERNELSYMBOLREADER_H_

#include "longoperation.h"
#include "kernelsymbolstream.h"

// forward declarations
class QIODevice;
class KernelSymbols;
struct MemSpecs;

/**
 * This class reads kernel symbols in a self-defined, compact format to a file
 * or any other QIODevice.
 */
class KernelSymbolReader: public LongOperation
{
public:
    /**
     * Constructor
     * @param from source to read the previsouly saved debugging symbols from
     * @param symbols the KernelSymbols to use for symbol creation
     * @param specs the MemSpecs to write the memory specifications to
     */
    KernelSymbolReader(QIODevice* from, KernelSymbols* symbols, MemSpecs* specs);

    /**
     * Starts the reading process
     * @exception ReaderWriterException error in data format
     * @exception GenericException out of memory
     */
    void read();

protected:
    /**
     * Displays progress information
     */
    virtual void operationProgress();

    void readVersion11(KernelSymbolStream& in);
    void readVersion12(KernelSymbolStream& in);

private:

    /// Encodes the reading phases of the reading process
    enum Phases {
        phCompileUnits,
        phElementaryTypes,
        phReferencingTypes,
        phStructuredTypes,
        phTypeRelations,
        phVariables,
        phCandidateTypes,
        phFinished
    };

    QIODevice* _from;
    KernelSymbols* _symbols;
    MemSpecs* _specs;
    Phases _phase;
};

#endif /* KERNELSYMBOLREADER_H_ */
