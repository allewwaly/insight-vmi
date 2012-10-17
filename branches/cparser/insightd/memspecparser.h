/*
 * memconstparser.h
 *
 *  Created on: 21.06.2010
 *      Author: chrschn
 */

#ifndef MEMSPECPARSER_H_
#define MEMSPECPARSER_H_

#include <QString>
#include "memspecs.h"
#include "genericexception.h"

#define memSpecParserError(x) do { throw MemSpecParserException((x), __FILE__, __LINE__); } while (0)
#define memSpecParserError2(x, o) do { throw MemSpecParserException((x), (o), __FILE__, __LINE__); } while (0)

/**
 * Exception class for MemSpecParser related errors
 */
class MemSpecParserException: public GenericException
{
public:
    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    MemSpecParserException(QString msg = QString(), const char* file = 0, int line = -1)
        : GenericException(msg, file, line)
    {
    }

    /**
      Constructor
      @param msg error message
      @param errorOutput the output of the failed command
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    MemSpecParserException(QString msg, QByteArray errorOutput,
                           const char* file = 0, int line = -1)
        : GenericException(msg, file, line), errorOutput(errorOutput)
    {
    }

    virtual ~MemSpecParserException() throw()
    {
    }

    virtual const char* className() const
    {
        return "MemSpecParserException";
    }

    QByteArray errorOutput;
};



/**
 * This class parses kernel specific memory constants from a kernel source tree
 * and a System.map file. These constants are required in order to correctly
 * interpret a memory dump.
 */
class MemSpecParser
{
public:
    /**
     * Constructor
     * @param kernelSrcDir path to the Linux source tree or headers
     * @param systemMapFile path to the \c System.map file of the same kernel
     */
    MemSpecParser(const QString& kernelSrcDir, const QString& systemMapFile);

    /**
     * Destructor
     */
    virtual ~MemSpecParser();

    /**
     * Parses the memory specifications from the previously given kernel source
     * tree and \c System.map file.
     * @return
     * @exception MemSpecParserException in case of errors
     */
    MemSpecs parse();

    /**
     * Returns the temporary build directory after it has been successfully
     * created by a call to parse(). Before that, an empty string is
     * returned.
     * @return the temporary build directory
     */
    QString buildDir() const;

    /**
     * Per default, the build directory is automatically removed when the
     * object is deleted.
     * @return \c true if the build directory is automatically removed if this
     * object is deleted, \c false otherwise
     */
    bool autoRemoveBuildDir() const;

    /**
     * Controls if the build directory is automatically removed when the object
     * is deleted.
     * @param autoRemove set to \c true to have the build directory removed,
     * \c false otherwise.
     */
    void setAutoRemoveBuildDir(bool autoRemove);

    /**
     * @return the output to standard error of the last failed process
     */
    const QByteArray& errorOutput() const;

private:
    /**
     * Creates a temporary build directory and sets up all necessary files.
     * \sa buildDir()
     */
    void setupBuildDir();

    /**
     * Builds the helper program in the directory \a buildDir.
     * @param specs the MemSpec struct to read architecture from
     */
    void buildHelperProg(const MemSpecs& specs);

    /**
     * Runs the helper program and parses the output.
     * @param specs the MemSpec struct to store the parsed result
     */
    void parseHelperProg(MemSpecs* specs);

    /**
     * Parses the \c System.map file and stores the relevant data to \a specs.
     * @param specs the MemSpec struct to store the parsed result
     */
    void parseSystemMap(MemSpecs* specs);


    /**
     * Parses the kernel's configuration file and determines the target
     * architecture: i386 or x86_64
     * @param specs the memory specification to store the architecture value
     */
    void parseKernelConfig(MemSpecs* specs);

    /**
     * Removes a directory and all contained files recursively.
     * @param dir the directory to remove
     * @return \c true in case of success, \c false otherwise
     */
    bool rmdirRek(const QString& dir) const;


    /**
     * Writes text \a src to file \a fileName
     * @param fileName
     * @param src
     * @throw MemSpecParserException in case the file cannot be written
     */
    void writeSrcFile(const QString& fileName, const QString& src);

    bool _autoRemoveBuildDir;
    QString _kernelSrcDir;
    QString _systemMapFile;
    QString _buildDir;
    QByteArray _errorOutput;
    quint64 _vmallocEarlyreserve;
};

#endif /* MEMSPECPARSER_H_ */
