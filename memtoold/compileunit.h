/*
 * compileunit.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef COMPILEUNIT_H_
#define COMPILEUNIT_H_

#include "symbol.h"

/**
 * Holds the infomation about a compilation unit, i.e., a source file.
 */
class CompileUnit: public Symbol
{
public:
    /**
     * Constructor
     */
    CompileUnit();

    /**
      Constructor
      @param info the type information to construct this type from
     */
    CompileUnit(const TypeInfo& info);

    /**
     * @return the directory name of this compilation unit
     */
	const QString& dir() const;

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

private:
    QString _dir; ///< Holds the directory name of this compilation unit
};


/**
 * Operator for native usage of the CompileUnit class for streams
 * @param in data stream to read from
 * @param unit object to store the serialized data to
 * @return the data stream \a in
 */
QDataStream& operator>>(QDataStream& in, CompileUnit& unit);


/**
 * Operator for native usage of the CompileUnit class for streams
 * @param out data stream to write the serialized data to
 * @param unit object to serialize
 * @return the data stream \a out
 */
QDataStream& operator<<(QDataStream& out, const CompileUnit& unit);


#endif /* COMPILEUNIT_H_ */
