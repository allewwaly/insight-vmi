/*
 * typefactory.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include <QPair>
#include <QList>
#include <QHash>
#include <QMultiHash>
#include <exception>

class BaseType;

#include "numeric.h"

/**
  Basic exception class for all factory-related exceptions
 */
class FactoryException : public std::exception
{
public:
    QString message;   ///< error message
    QString file;      ///< file name in which message was originally thrown
    int line;          ///< line number at which message was originally thrown

    /**
      Constructor
      @param msg error message
      @param file file name in which message was originally thrown
      @param line line number at which message was originally thrown
      @note Try to use @c __FILE__ for @a file and @c __LINE__ for @a line.
     */
    FactoryException(QString msg = QString(), const char* file = 0, int line = -1)
        : message(msg), file(file), line(line)
    {
    }

    virtual ~FactoryException() throw()
    {
    }
};


/// List of all BaseType elements
typedef QList<BaseType*> BaseTypeList;

/// Key for hashing elements of BaseType based on their name and size
typedef QPair<QString, quint32> BaseTypeHashKey;

/// Hash table for BaseType pointers
typedef QMultiHash<BaseTypeHashKey, BaseType*> BaseTypeKeyHash;

/// Hash table for BaseType pointers
typedef QMultiHash<QString, BaseType*> BaseTypeStringHash;

/// Hash table to find all types by ID
typedef QHash<int, BaseType*> BaseTypeIntHash;

/// This function is required to use BaseType as a key in a QHash
inline uint qHash(const BaseTypeHashKey &key)
{
    return qHash(key.first) ^ key.second;
}

/**
 * Creates and holds all defined types
 */
class TypeFactory
{
public:
	/// The data encoding of a type
	enum Encoding { eUndef = 0, eSigned, eUnsigned, eFloat };

	TypeFactory();

	~TypeFactory();

	BaseType* findById(int id) const;

	BaseType* findByName(const QString& name) const;

	BaseType* create(const QString& name, size_t size, int id);

	template<class T>
	inline T* getInstance(const QString& name, int id, quint32 size,
			QIODevice *memory = 0)
	{
		BaseType* t = findById(id);
		if (!t) {
			t = new T(name, id, size, memory);
			insert(t);
		}
		return dynamic_cast<T*>(t);
	}

	BaseType* getNumericInstance(const QString& name, int id, quint32 size,
			Encoding enc, QIODevice *memory = 0);

	inline const BaseTypeList& types() const
	{
		return _types;
	}

protected:
	void insert(BaseType* type);

private:
	BaseTypeList _types;          ///< Holds all BaseType objects
	BaseTypeStringHash _nameHash; ///< Holds all BaseType objects, indexed by name
	BaseTypeIntHash _idHash;      ///< Holds all BaseType objects, indexed by ID
};

#endif /* TYPEFACTORY_H_ */
