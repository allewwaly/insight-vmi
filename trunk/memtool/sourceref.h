/*
 * sourceref.h
 *
 *  Created on: 03.04.2010
 *      Author: chrschn
 */

#ifndef SOURCEREF_H_
#define SOURCEREF_H_

class SourceRef
{
public:
	/**
	 * Constructor
	 * @param srcFile
	 * @param srcLine
	 * @return
	 */
	SourceRef(int srcFile = -1, int srcLine = -1);

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

protected:
	int _srcFile;        ///< ID of the source file of the type's declaration
	int _srcLine;        ///< line number within the source the type's declaration
};

#endif /* SOURCEREF_H_ */
