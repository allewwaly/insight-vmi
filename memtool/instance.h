/*
 * instance.h
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <QObject>
#include <QString>
#include <QStringList>

class BaseType;
class VirtualMemory;
class Instance;

/// A list of Instance objects
typedef QList<Instance> InstanceList;

/**
 * This class wraps a variable instance in a memory dump.
 */
class Instance
{
public:
	/**
	 * Constructor
	 */
	Instance();

	/**
	 * Constructor
	 * @param address the address of that instance
	 * @param type the underlying BaseType of this instance
	 * @param name the name of this instance
	 * @param parentName the full name of the parent
	 * @param vmem the virtual memory device to read data from
	 */
	Instance(size_t address, const BaseType* type, const QString& name,
			const QString& parentName, VirtualMemory* vmem, int id = -1);

	/**
	 * Destructor
	 */
	virtual ~Instance();

	/**
	 * @return the ID of this instance, if it is a variable instance, -1 otherwise
	 */
	int id() const;

	/**
	 * @return the virtual address of the variable in memory
	 */
	quint64 address() const;

	/**
	 * This gives you the (short) name of this Instance, i. e., its name its
	 * parent's struct, e. g. \c next.
	 * @return the name of this instance
	 */
	QString name() const;

	/**
	 * This function returns the full name of the parent's struct, as it was
	 * found, e. g. \c init_task.children
	 * @return the full name of the parent's struct
	 */
	QString parentName() const;

	/**
	 * Use this function to retrieve the full name of this instance as it was
	 * found following the names and members of structs in dotted notation,
	 * i.e., \c init_task.children.next.
	 * @return the full name of this instance
	 */
	QString fullName() const;

	/**
	 * Gives access to the names of all members if this instance.
	 * @return the names of all direct members of this instance
	 */
	const QStringList& memberNames() const;

	/**
	 * @return a list of instances of all members
	 */
	InstanceList members() const;

	/**
	 * Gives access to the concrete BaseType of this instance.
	 * @return
	 */
	const BaseType* type() const;

	/**
	 * Convenience function to access type()->name()
	 * @return the name of this instance's type
	 */
	QString typeName() const;

	/**
	 * Convenience function to access type()->size()
	 * @return the size of this instance's type
	 */
	quint32 size() const;

	/**
	 * Checks if this is a valid instance and its address is not null.
	 * @return \c true if this object is null, \c false otherwise
	 * \sa isValid()
	 */
	bool isNull() const;

    /**
     * Checks if this is a valid instance and its address is not null.
     * @return \c true if this object is valid, \c false otherwise
     * \warning This does \e not check if the address is null or not!
     * \sa isNull()
     */
	bool isValid() const;

	/**
	 * @return the number of members, if this is a struct, \c 0 otherwise
	 */
	int memberCount() const;

	/**
	 * Access function to the members of this instance, if it is a struct. If
	 * this instance is no struct/union or if the index is out of bounds, a
	 * null Instance is returned.
	 *
	 * This function is much more efficient than getting the whole list of
	 * members with the members() function and than accessing an individual
	 * member.
	 *
	 * \note Make sure to check Instance::isNull() on the returned object to
	 * see if it is valid or not.
	 *
	 * @param index index into the member list
	 * @return
	 */
	Instance member(int index) const;

	/**
	 * Checks if a member with the given name \a name exists in this instance.
	 * @param name the name of the member to find
	 * @return \c true if that member exists, \c false otherwise
	 */
	bool memberExists(const QString& name) const;

	/**
	 * Retrieves a member (i.e., struct components) of this Instance, if it
	 * exists. You can check their existence with memberExists() or by iterating
	 * over the names returned by memberNames().
	 *
	 * \note Make sure to check Instance::isNull() on the returned object to
	 * see if it is valid or not.
	 *
	 * @param name the name of the member to find
	 * @return a new Instance object if the member was found, or an empty
	 * object otherwise
	 */
	Instance findMember(const QString& name) const;

	/**
	 * Retrieves the index of the member with name \a name. This index can be
	 * used as array index to both members() and memberNames().
	 * @param name the name of the member to find
	 * @return the index of that member, if found, or \c -1 otherwise.
	 */
	int indexOfMember(const QString& name) const;

	/**
	 * Retrieves the type ID of the member \a name.
	 * @param name the name of the member
	 * @return the ID of the type, if that member exists, \c 0 otherwise.
	 */
	int typeIdOfMember(const QString& name) const;

    /**
     * Explicit representation of this instance as qint8.
     * @return the value of this type as a qint8
     */
    qint8 toInt8() const;

    /**
     * Explicit representation of this instance as quint8.
     * @return the value of this type as a quint8
     */
    quint8 toUInt8() const;

    /**
     * Explicit representation of this instance as qint16.
     * @return the value of this type as a qint16
     */
    qint16 toInt16() const;

    /**
     * Explicit representation of this instance as quint16.
     * @return the value of this type as a quint16
     */
    quint16 toUInt16() const;

    /**
     * Explicit representation of this instance as qint32.
     * @return the value of this type as a qint32
     */
    qint32 toInt32() const;

    /**
     * Explicit representation of this instance as quint32.
     * @return the value of this type as a quint32
     */
    quint32 toUInt32() const;

    /**
     * Explicit representation of this instance as qint64.
     * @return the value of this type as a qint64
     */
    qint64 toInt64() const;

    /**
     * Explicit representation of this instance as quint64.
     * @return the value of this type as a quint64
     */
    quint64 toUInt64() const;

    /**
     * Explicit representation of this instance as float.
     * @return the value of this type as a float
     */
    float toFloat() const;

    /**
     * Explicit representation of this instance as double.
     * @return the value of this type as a double
     */
    double toDouble() const;

    /**
     * Explicit representation of this instance as a pointer.
     * @return the value of this type as a variant
     * @warning This function should only be called for a pointer type!
     * @warning The pointer has the bit size of the host system! Use
     * pointerSize() to retrieve the pointer size of the guest system.
     */
    void* toPointer() const;

    /**
     * Explicit representation of this instance as QVariant.
     * @return the value of this type as a variant
     */
    template<class T>
    QVariant toVariant() const;
//    {
//        return value<T>(mem, offset);
//    }

    /**
     * @return a string representation of this instance
     */
    QString toString() const;

    /**
     * @return the size of a pointer for this instance in bytes
     */
    int pointerSize() const;

private:
	static const QStringList _emtpyStringList;
	int _id;
	size_t _address;
	const BaseType* _type;
	QString _name;
	QString _parentName;
    VirtualMemory* _vmem;
    bool _isNull;
    bool _isValid;
};

#endif /* INSTANCE_H_ */
