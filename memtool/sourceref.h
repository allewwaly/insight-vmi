/*
 * sourceref.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef SOURCEREF_H_
#define SOURCEREF_H_

#include "typeinfo.h"
#include <QDataStream>

class SourceRef
{
public:
	/**
	 * Constructor
	 * @param srcFile
	 * @param srcLine
	 * @return
	 */
	SourceRef(const TypeInfo& info);

    /**
      @return the ID of the source file in which this type was declared
     */
    int srcFile() const;

    /**
      Sets the ID of the source file in which this type was declared
      @param id new ID
     */
    void setSrcFile(int id);

    /**
      @return the line number at which this type was declared
     */
    int srcLine() const;

    /**
      Sets the line number at which this type was declared
      @param line the new line number
     */
    void setSrcLine(int line);

    /**
     * Reads a serialized version of this object from \a in.
     * \sa writeTo()
     * @param in the data stream to read the data from, must be ready to read
     */
    virtual void readFrom(QDataStream& in);

    /**
     * Writes a serialized version of this object to \a out
     * \sa readFrom()
     * @param out the data stream to write the data to, must be ready to write
     */
    virtual void writeTo(QDataStream& out) const;

protected:
	int _srcFile;        ///< ID of the source file of the type's declaration
	int _srcLine;        ///< line number within the source the type's declaration
};

#endif /* SOURCEREF_H_ */
