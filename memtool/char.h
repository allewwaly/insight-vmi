/*
 * char.h
 *
 *  Created on: 29.03.2010
 *      Author: chrschn
 */

#ifndef CHAR_H_
#define CHAR_H_

#include "basetype.h"

template<class T>
class GenericChar: public BaseType
{
public:
    /**
      Constructor
      @see BaseType::BaseType()
     */
    GenericChar(const QString& name, const quint32 id, const quint32 size,
            QIODevice *memory);

//  virtual ~GenericChar();

    /**
      @return the native value of this type
     */
    T toChar(size_t offset) const;

    /**
      @return a string representation of this type
     */
    virtual QString toString(size_t offset) const;

    /**
      @return the value of this type as a variant
     */
    virtual QVariant value(size_t offset) const;

};

//------------------------------------------------------------------------------
class Char: public GenericChar<char>
{
public:
    /**
      Constructor
      @see BaseType::BaseType()
     */
	Char(const quint32 id, const quint32 size, QIODevice *memory);

    /**
      @return the actual type of that polimorphic instance
     */
    virtual RealType type() const;
};

//------------------------------------------------------------------------------
class UnsignedChar: public GenericChar<unsigned char>
{
public:
    /**
      Constructor
      @see BaseType::BaseType()
     */
    UnsignedChar(const quint32 id, const quint32 size, QIODevice *memory);

    /**
      @return the actual type of that polimorphic instance
     */
    virtual RealType type() const;
};

#endif /* CHAR_H_ */
