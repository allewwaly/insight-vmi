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
#include <QSharedDataPointer>
#include "instancedata.h"

class BaseType;
class VirtualMemory;
class Instance;
//class InstanceData;

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
	 * @param parentNames the full name of the parent
	 * @param vmem the virtual memory device to read data from
     * @param id the ID of the variable this instance represents, if any
	 */
	Instance(size_t address, const BaseType* type, const QString& name,
            const QStringList& parentNames, VirtualMemory* vmem, int id = -1);

    /**
     * Copy constructor
     * @param other object to copy from
     */
    Instance(const Instance& other);

	/**
	 * Destructor
	 */
	virtual ~Instance();

	/**
	 * @return the ID of this instance, if it is a variable instance, -1 otherwise
	 */
	int id() const;

    /**
     * @return the array index of the memory dump this instance belongs to
     */
    int memDumpIndex() const;

	/**
	 * @return the virtual address of the variable in memory
	 */
	quint64 address() const;

	/**
	 * Sets the virtual addres of the variable in memory
	 * @param addr the new virtual address
	 */
	void setAddress(quint64 addr);

	/**
	 * Adds \a offset to the virtual address.
	 * @param offset the offset to add
	 */
	void addToAddress(quint64 offset);

	/**
	 * This gives you the (short) name of this Instance, i. e., its name its
	 * parent's struct, e. g. \c next.
	 * @return the name of this instance
	 */
	QString name() const;

	/**
	 * Sets the name of this instance
	 * @param name new name for this instance
	 */
	void setName(const QString& name);

	/**
	 * This function returns the full name of the parent's struct, as it was
	 * found, e. g. \c init_task.children
	 * @return the full name of the parent's struct
	 */
	QString parentName() const;

	/**
	 * This function returns all component names of the parent's struct, as it
	 * was found, e. g. \c init_task, \c children
	 * @return a list of the name components of the parent's struct
	 */
	QStringList parentNameComponents() const;

	/**
	 * Use this function to retrieve the full name of this instance as it was
	 * found following the names and members of structs in dotted notation,
	 * i.e., \c init_task.children.next.
	 * @return the full name of this instance
	 */
	QString fullName() const;

    /**
     * Use this function to retrieve all components of the full name of this
     * instance as it was found following the names and members of structs.
     * @return the full name components of this instance
     */
	QStringList fullNameComponents() const;

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
	 * Changes the type of this instance to the provided type.
	 * @param type the new type for this instance
	 */
	void setType(const BaseType* type);

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
     * Checks if this instance has type.
     * @return \c true if this object has a type, \c false otherwise
     * \warning This does \e not check if the address is null or not!
     * \sa isNull()
     */
	bool isValid() const;

	/**
	 * Checks if this instance is accessible, i. e., that its value can be read
	 * from the memory dump through its virtual address.
	 * @return \c true if the instance is accessible, \c false otherwise
	 */
	bool isAccessible() const;

	/**
	 * Compares this Instance with \a other on a value basis. Two instances
	 * must have the same BaseType as returned by type() to possibly be equal.
	 * In addition, their following contents is compared to determine their
	 * equality:
	 *
	 * \li NumericBaseType: the numeric value
	 * \li Enum: the enumeration value
	 * \li FuncPointer: the function's virtual address
	 * \li Array: the array values, if the array length is known
	 * \li untyped Pointer (void *): the virtual address pointed to
     * \li For any other RefBaseType (i.e., ConstType, Pointer, Typedef,
     *      VolatileType), the equality decision is delegated to the referenced
     *      type's equals() function.
	 * \li Structured: all members of the above mention types are compared,
	 *      but nested structs are ignored
	 *
	 * @param other the Instance object to compare this instance to
	 * @return \c true if the two instances are considered equal, \c false
	 * otherwise
	 */
	bool equals(const Instance& other) const;

    /**
     * Compares this struct or union Instance with \a other using the equals()
     * function and returns a list of member names that are different.
     *
     * If this instance is not a struct or union or is not of the same type as
     * \a other, then a QStringList with just a single, empty string is
     * returned.
     *
     * If this instance equals \a other for all members, then an empty
     * QStringList is returned.
     *
     * @param other the Instance object to compare this instance to
     * @param recursive if \c true, this function recurses into nested structs
     *      or unions
     * @return \c true if the two instannces are considered equal, \c false
     * otherwise
     */
	QStringList differences(const Instance& other, bool recursive) const;

	/**
	 * Treats this Instance as an array instance and returns a new instance
	 * of the same type at the memory position as this instance plus \a index
	 * times the size of the array's value type. If this is not a Pointer or an
	 * Array instance, then an invalid Instance is returned
	 *
	 * \warning The length() parameter of the underlying Array type is neither
	 * checked against \a index nor modified. Be careful to only use the
	 * length() parameter of the original instance (with array index 0), not on
	 * any Instance object returned by this function.
	 *
	 * @param index array index to access
	 * @return a Instance at memory position size() + \a index *
	 * type()->refType()->size(), if this is a Pointer or an Array instance,
	 * otherwise an empty Instance object
	 */
	Instance arrayElem(int index) const;

	/**
	 * Dereferences this instance as far as possible.
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
	 * @param derefCount pointer to a counter variable for how many types have
	 * been followed to create the instance
	 * @return a dereferenced version of this instance
	 * \sa BaseType::TypeResolution
	 */
	Instance dereference(int resolveTypes, int* derefCount = 0) const;

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
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
	 * @return Instance object of the specified member
     * \sa BaseType::TypeResolution
	 */
	Instance member(int index, int resolveTypes) const;

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
     * @param resolveTypes which types to automatically resolve, see
     * BaseType::TypeResolution
	 * @return a new Instance object if the member was found, or an empty
	 * object otherwise
     * \sa BaseType::TypeResolution
	 */
	Instance findMember(const QString& name, int resolveTypes) const;

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

    static qint64 objectCount();

private:
    typedef QSet<quint64> VisitedSet;

    void differencesRek(const Instance& other, const QString& relParent,
            bool includeNestedStructs, QStringList& result,
            VisitedSet& visited) const;

	QSharedDataPointer<InstanceData> _d;

	static qint64 _objectCount;
};

#endif /* INSTANCE_H_ */
