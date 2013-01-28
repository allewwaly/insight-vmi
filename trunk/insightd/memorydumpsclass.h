/*
 * memorydumpsclass.h
 *
 *  Created on: 09.08.2011
 *      Author: chrschn
 */

#ifndef MEMORYDUMPSCLASS_H_
#define MEMORYDUMPSCLASS_H_

#include <QObject>
#include <QScriptable>
#include <QScriptValue>

class KernelSymbols;

/**
 * This class allows management of memory files within the scripting engine.
 *
 * All methods of this class implemented as Qt slots are available within
 * the scripting environment using the global \c Memory object. See the list()
 * method as an example.
 */
class MemoryDumpsClass: public QObject, public QScriptable
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent parent QObject
     */
    MemoryDumpsClass(KernelSymbols* symbols, QObject* parent = 0);

    /**
     * Destructor
     */
    virtual ~MemoryDumpsClass();

public slots:
    /**
     * Lists all memory files that are currently loaded.
     *
     * \warning The returned array is not necessarily zero-based, the file names
     * are indexed in the order as they are loaded and unloaded!
     *
     * @return an array of file names loaded
     *
     * Example usage in a script:
     * \code
     * function listFiles()
     * {
     *     var list = Memory.list();
     *     for (var i in list)
     *         print("[" + i + "] => " + list[i]);
     * }
     *
     * Memory.load("file1.bin");
     * Memory.load("file2.bin");
     *
     * listFiles();
     * // output:
     * // [0] => file1.bin
     * // [1] => file1.bin
     *
     * Memory.unload(0);
     * // output:
     * // [1] => file1.bin
     *
     * Memory.load("file3.bin");
     * // output:
     * // [0] => file3.bin
     * // [1] => file1.bin
     * \endcode
     *
     * \sa load(), unload()
     */
    QScriptValue list() const;

    /**
     * Loads a memory file.
     * @param fileName the file to load
     * @return In case of success, the index of the newly load file into the
     * list of files is returned, so the value is greater or equal than zero. If
     * the loading fails (e. g., because the file was not found), an error code
     * less than zero is returned.
     * \sa list(), unload()
     */
    int load(const QString& fileName) const;

    /**
     * Unloads a memory file, either based on the file name or an the index into
     * the list of loaded files.
     * @param indexOrfileName either the name or the list index of the file to
     * unload
     * @return In case of success, the index of the unload file is returned, so
     * the value is greater or equal than zero. If the operation fails (e. g.,
     * invalid index), an error code less than zero is returned.
     * \sa list(), load()
     */
    int unload(const QString& indexOrfileName) const;
private:
    KernelSymbols* _symbols;
};

#endif /* MEMORYDUMPSCLASS_H_ */
