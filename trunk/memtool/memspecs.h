/*
 * memspecs.h
 *
 *  Created on: 21.06.2010
 *      Author: chrschn
 */

#ifndef MEMSPECS_H_
#define MEMSPECS_H_

#include <QDataStream>

/**
 * This struct holds the definition of how a memory specification can be
 * extracted from a kernel source tree. It is used in conjunction with the
 * MemSpecs and MemSpecsParser classes.
 *
 * For this purpose, a small helper program
 * is compiled against the configured kernel source tree and then outputs the
 * desired information as a key-value pair in a specified printf() format.
 *
 * Suppose we want to know the value of the macro \c __PAGE_OFFSET of a kernel.
 * First, we set keyFmt to any self-explaining name, e. g.,
 * keyFmt="PAGE_OFFSET". This will be the key we have to look for in
 * MemSpecs::setFromKeyValue().
 *
 * Next, we have to define how the value can be
 * calculated from the kernel source in valueFmt. This can be any valid C
 * expression, including pre-processor macros. So for our example we set
 * valueFmt="__PAGE_OFFSET".
 *
 * Finally, we have to specify how the value should be output as string. This
 * is done by setting outputFmt to any valid format that printf will accept,
 * e. g., outputFmt="%16lx".
 *
 * To continue our example, the MemSpecsParser class will then create a helper
 * program which consists of the statement:
 *
 * \code
 * printf("PAGE_OFFSET=%16lx\n", __PAGE_OFFSET);
 * \endcode
 *
 * This program is compiled and run and the output is parsed and handed over
 * to a MemSpecs object by calling MemSpecs::setFromKeyValue().
 */
struct KernelMemSpec
{
    /// Constructor
    KernelMemSpec() {}

    /// Constructor
    KernelMemSpec(QString keyFmt, QString valueFmt, QString outputFmt) :
        keyFmt(keyFmt),
        valueFmt(valueFmt),
        outputFmt(outputFmt)
    {}

    /// The identifier (the key) of a key-value pair.
    QString keyFmt;

    /// A valid expression in C syntax to calcluate the value of a key-value pair.
    QString valueFmt;

    /// The output format in which the value should be printed (printf syntax).
    QString outputFmt;
};


/// A list of KernelMemSpec objects
typedef QList<KernelMemSpec> KernelMemSpecList;


/**
 * This struct holds the memory specifications, i.e., the fixed memory offsets
 * and locations as well as the CPU architecture.
 */
struct MemSpecs
{
    /// Architecture variants: i386 or x86_64
    enum Architecture { i386, x86_64 };

    /// Constructor
    MemSpecs() :
        pageOffset(0),
        vmallocStart(0),
        vmallocEnd(0),
        vmemmapStart(0),
        vmemmapEnd(0),
        modulesVaddr(0),
        modulesEnd(0),
        startKernelMap(0),
        initLevel4Pgt(0),
        sizeofUnsignedLong(sizeof(unsigned long)),
        arch(x86_64)
    {}

    /**
     * Sets any of the local member variables by parsing a key-value pair.
     * @param key the variable identifier
     * @param value the new value as string
     * @return \c true if the key-value pair could be parsed correctly,
     * \c false otherwise
     * \sa KernelMemSpec
     * \sa supportedMemSpecs()
     */
    bool setFromKeyValue(const QString& key, const QString& value);

    /**
     * @return a list of all kernel memory specifications that are supported
     * by setFromKeyValue().
     * \sa setFromKeyValue()
     */
    static KernelMemSpecList supportedMemSpecs();

    quint64 pageOffset;
    quint64 vmallocStart;
    quint64 vmallocEnd;
    quint64 vmemmapStart;
    quint64 vmemmapEnd;
    quint64 modulesVaddr;
    quint64 modulesEnd;
    quint64 startKernelMap;
    quint64 initLevel4Pgt;
    int sizeofUnsignedLong;
    Architecture arch;
};


/**
* Operator for native usage of the Variable class for streams
* @param in data stream to read from
* @param specs object to store the serialized data to
* @return the data stream \a in
*/
QDataStream& operator>>(QDataStream& in, MemSpecs& specs);


/**
* Operator for native usage of the Variable class for streams
* @param out data stream to write the serialized data to
* @param specs object to serialize
* @return the data stream \a out
*/
QDataStream& operator<<(QDataStream& out, const MemSpecs& specs);


#endif /* MEMSPECS_H_ */
