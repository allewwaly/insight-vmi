/*
 * instanceprototype.h
 *
 *  Created on: 05.07.2010
 *      Author: chrschn
 */

#ifndef INSTANCEPROTOTYPE_H_
#define INSTANCEPROTOTYPE_H_

#include <QObject>
#include <QScriptable>
#include <QStringList>
#include "instance.h"
#include "genericexception.h"
#include "instancedata.h"


/**
 * This class is the prototype for script variables of type Instance within the
 * QtScript environment.
 *
 * All methods of this class that are implemented as slots and are thus
 * accessible from Instance objects within the scripting environment. Most of
 * the methods just forward the call to the encapsulated Instance object,
 * which is obtained using the private method thisInstance().
 *
 * The ECMA-262 scripting language specification (e. g., JavaScript, QtScript)
 * only supports the integer types \c int32 and \c uint32. Any \c int64 or
 * \c uint64 value in QtScript is automatically casted to a \c double value,
 * which leads to a loss of precision. Most notably, this affects the virtual
 * address of this object on 64-bit kernels, but also <tt>long long</tt>
 * integers on a 32-bit kernel. In these cases, the address cannot be accessed
 * in their full length with script-native integer types.  We handle this
 * problem by providing three getter and three setter methods for the address:
 *
 * \li Address() returns the address as zero-padded string in hex format
 * \li SetAddress() sets the address based on string in hex format
 * \li AddressHigh() returns the most significant 32 bits of the address as
 *      an \c uint32 value
 * \li SetAddressHigh() sets the most significant 32 bits of the address
 * \li AddressLow() returns the least significant 32 bits of the address as
 *      an \c uint32 value
 * \li SetAddressLow() sets the least significant 32 bits of the address
 *
 * The access method for <tt>long long</tt> integer values comes with three
 * getter and three setter methods in the same fashion.
 *
 * An Instance object within the scripting environment can be retrieved by
 * passing the name of a global variable to the constructor of an Instance
 * object, for example:
 * \code
 * var init = new Instance("init_task");
 * \endcode
 *
 * You can now use all the methods of the InstancePrototype class to interact
 * with this object:
 * \code
 * // Retrieve information about the instance
 * print("init.Address() = 0x" + init.Address());
 * print("init.FullName() = " + init.FullName());
 * print("init.Type() = " + init.Type());
 * print("init.TypeName() = " + init.TypeName());
 * print("init.Size() = " + init.Size());
 * \endcode
 *
 * If the instance represents a struct or a union, it members can be accessed
 * using the Members() method. This method returns an array of Instance
 * objects of all members of the callee:
 * \code
 * // Iterate over all members using an array of Instance objects
 * var members = init.Members();
 * for (var i = 0; i < members.length; ++i) {
 *     var offset = init.MemberOffset(members[i].Name());
 *     print ((i+1) +
 *             ". 0x" + offset.toString(16) + " " +
 *             members[i].Name() + ": " +
 *             members[i].TypeName() +
 *             " @ 0x" + members[i].Address());
 * }
 * \endcode
 *
 * The members can also be accessed using a JavaScript-style iterator:
 * \code
 * // Iterate over all members using an iterator
 * var i = 1;
 * for (m in init) {
 *     var offset = init.MemberOffset(init[m].Name());
 *     print ((i++) +
 *             ". 0x" + offset.toString(16) + " " +
 *             init[m].Name() + ": " +
 *             init[m].TypeName() +
 *             " @ 0x" + init[m].Address());
 * }
 * \endcode
 *
 * In addition, if the name of a member is known, it can be directly accessed
 * as a property of that Instance object:
 * \code
 * // Print a list of running processes
 * var proc = init;
 * do {
 *     print(proc.pid + "  " + proc.comm);
 *     proc = proc.tasks.next;
 * } while (proc.Address() != init.Address());
 * \endcode
 *
 * \note In order to prevent name collisions between Instance members and the
 * methods provided by the InstancePrototype class, all methods start with
 * an upper-case letter. The only exception to this rule are the explicit
 * casting methods, such as toString(), toInt32(), toFloat() etc.
 *
 * \sa Instance
 * \sa InstanceClass
 */
class InstancePrototype : public QObject, public QScriptable
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent parent object, defaults to 0
     */
    InstancePrototype(QObject *parent = 0);

    /**
     * Destructor
     */
    virtual ~InstancePrototype();

public slots:
	/**
	 * Retrieves the virtual address of this instance.  As JavaScript does
     * not support native 64-bit integers, the value is returned as a
     * hex-encoded string.
	 * @return the address as zero-padded string in hex format
     * \sa AddressLow(), AddressHigh(), SetAddress()
	 */
    QString Address() const;

    /**
     * Sets the address of this instance to the given address. The address must
     * be given as hex encoded string, which may optionally start with
     * "0x".
     * @param addr the hex enocded address to set
     * \sa SetAddressLow(), SetAddressHigh(), Address()
     */
    void SetAddress(QString addr);

    /**
     * @return the most significant 32 bits of the address as \c uint32
     * \sa AddressLow(), Address(), SetAddressHigh()
     */
    quint32 AddressHigh() const;

    /**
     * Sets the most significant 32 bits of the address.
     * @param addrHigh the most significant 32 bits of the address
     * \sa SetAddressLow(), SetAddress(), AddressHigh()
     */
    void SetAddressHigh(quint32 addrHigh);

    /**
     * @return the least significant 32 bits of the address as \c uint32
     * \sa AddressHigh(), Address(), SetAddressLow()
     */
    quint32 AddressLow() const;

    /**
     * Sets the least significant 32 bits of the address.
     * @param addrLow the least significant 32 bits of the address
     * \sa SetAddressHigh(), SetAddress(), AddressLow()
     */
    void SetAddressLow(quint32 addrLow);

    /**
     * Adds \a offset to the virtual address. To subtract an offset, just supply
     * a negative value for \a offset.
     * @param offset the offset to add
     * \sa Address(), SetAddress()
     */
    void AddToAddress(int offset);

    /**
     * @return the ID of this instance, if it was directly instantiated from a
     * global variable, -1 otherwise
     */
    int Id() const;

    /**
     * @return the array index of the memory dump this instance belongs to
     */
    int MemDumpIndex() const;

    /**
     * This gives the (short) name of this Instance, i. e., its name its
     * parent's struct. For example:
     * \code
     * var i = new Instance("init_task.tasks.next");
     * print(i.Name()); // output: next
     * \endcode
     * @return the name of this instance
     * \sa SetName(), ParentName(), FullName()
     */
    QString Name() const;

    /**
     * This method returns the full name of the parent's struct, as it was
     * found. For example:
     * \code
     * var i = new Instance("init_task.tasks.next");
     * print(i.Name()); // output: init_task.tasks
     * \endcode
     * @return the full name of the parent's struct
     * \sa Name(), FullName()
     */
    QString ParentName() const;

    /**
     * Use this method to retrieve the full name of this instance as it was
     * found following the names and members of structs in dotted notation,
     * for example:
     * \code
     * var i = new Instance("init_task.tasks.next");
     * print(i.Name()); // output: init_task.tasks.next
     * \endcode
     * @return the full name of this instance
     * \sa Name(), ParentName()
     */
    QString FullName() const;

    /**
     * Gives access to the names of all members if this instance.
     * @return a list of the names of all direct members of this instance
     * \sa Members(), MemberExists()
     */
    QStringList MemberNames() const;

    /**
     * Gives access to the names of all members if this instance.
     * @return a list of instances of all members
     * \sa MemberNames(), FindMember()
     */
    InstanceList Members() const;

    /**
     * Retrieves the real type of this instance, as defined by
     * BaseType::RealType, as a string. The following table lists the possible
     * types and corresponding return values:
     *
     * <table border="1">
     * <tr><th><tt>BaseType::RealType</tt></th><th>String</th></tr>
     * <tr><td><tt>BaseType::rtInt8</tt></td><td><tt>"Int8"</tt></td></tr>
     * <tr><td><tt>BaseType::rtUInt8</tt></td><td><tt>"UInt8"</tt></td></tr>
     * <tr><td><tt>BaseType::rtBool8</tt></td><td><tt>"Bool8"</tt></td></tr>
     * <tr><td><tt>BaseType::rtInt16</tt></td><td><tt>"Int16"</tt></td></tr>
     * <tr><td><tt>BaseType::rtUInt16</tt></td><td><tt>"UInt16"</tt></td></tr>
     * <tr><td><tt>BaseType::rtBool16</tt></td><td><tt>"Bool16"</tt></td></tr>
     * <tr><td><tt>BaseType::rtInt32</tt></td><td><tt>"Int32"</tt></td></tr>
     * <tr><td><tt>BaseType::rtUInt32</tt></td><td><tt>"UInt32"</tt></td></tr>
     * <tr><td><tt>BaseType::rtBool32</tt></td><td><tt>"Bool32"</tt></td></tr>
     * <tr><td><tt>BaseType::rtInt64</tt></td><td><tt>"Int64"</tt></td></tr>
     * <tr><td><tt>BaseType::rtUInt64</tt></td><td><tt>"UInt64"</tt></td></tr>
     * <tr><td><tt>BaseType::rtBool64</tt></td><td><tt>"Bool64"</tt></td></tr>
     * <tr><td><tt>BaseType::rtFloat</tt></td><td><tt>"Float"</tt></td></tr>
     * <tr><td><tt>BaseType::rtDouble</tt></td><td><tt>"Double"</tt></td></tr>
     * <tr><td><tt>BaseType::rtPointer</tt></td><td><tt>"Pointer"</tt></td></tr>
     * <tr><td><tt>BaseType::rtArray</tt></td><td><tt>"Array"</tt></td></tr>
     * <tr><td><tt>BaseType::rtEnum</tt></td><td><tt>"Enum"</tt></td></tr>
     * <tr><td><tt>BaseType::rtStruct</tt></td><td><tt>"Struct"</tt></td></tr>
     * <tr><td><tt>BaseType::rtUnion</tt></td><td><tt>"Union"</tt></td></tr>
     * <tr><td><tt>BaseType::rtTypedef</tt></td><td><tt>"Typedef"</tt></td></tr>
     * <tr><td><tt>BaseType::rtConst</tt></td><td><tt>"Const"</tt></td></tr>
     * <tr><td><tt>BaseType::rtVolatile</tt></td><td><tt>"Volatile"</tt></td></tr>
     * <tr><td><tt>BaseType::rtFuncPointer</tt></td><td><tt>"FuncPointer"</tt></td></tr>
     * </table>
     *
     * @return the BaseType::RealType of this instance, as a string
     */
    QString Type() const;

    /**
     * Retrieves the ID of the type of this instance.
     * @return the ID of this instance's type
     */
    int TypeId() const;

    /**
     * Retrieves the pretty-printed name of the type of this instance, for
     * example, <tt>unsigned int</tt>, <tt>struct list_head *</tt>, or
     * <tt>const int[16]</tt>.
     * @return the name of this instance's type
     */
    QString TypeName() const;

    /**
     * Returns the value of \c sizeof(type) for this instance's type.
     * @return the size of this instance's type
     */
    quint32 Size() const;

    /**
     * Checks if a member with the given name \a name exists in this instance.
     * @param name the name of the member to find
     * @return \c true if that member exists, \c false otherwise
     * \sa MemberNames()
     */
    bool MemberExists(const QString& name) const;

    /**
     * Calculates the offset of a member within a struct, if this is a struct
     * or union.
     * @param name the name of the member
     * @return offset of member \a name within the struct, or 0 if no such
     * member exists or if this instance is no struct or union
     */
    int MemberOffset(const QString& name) const;

    /**
     * Retrieves a member of this Instance, if it exists. You can check their
     * existence with MemberExists() or by iterating over the names returned by
     * MemberNames().
     *
     * \note Make sure to check IsValid() on the returned object to see if it is
     * valid or not.
     *
     * @param name the name of the member to find
     * @return a new Instance object if the member was found, or an empty
     * object otherwise
     * \sa MemberExists(), MemberNames(), IsValid()
     */
    Instance FindMember(const QString& name) const;

    /**
     * Retrieves the type ID of the member \a name.
     * @param name the name of the member
     * @return the ID of the type, if that member exists, \c 0 otherwise.
     */
    int TypeIdOfMember(const QString& name) const;

    /**
     * This method always returns 4 on 32-bit kernels and 8 on 64-bit kernels.
     * @return the size of pointers for this architecture, in bytes
     */
    int PointerSize() const;

    /**
     * Compares this Instance with \a other on a value basis. Two instances
     * must have the same BaseType as returned by Type() to potentially be
     * equal. In addition, their following contents is compared to determine
     * their equality:
     *
     * \li NumericBaseType: the numeric value
     * \li Enum: the enumeration value
     * \li FuncPointer: the function's virtual address
     * \li Array: the array values, if the array length is known
     * \li untyped Pointer (void *): the virtual address pointed to
     * \li For any other RefBaseType (i.e., ConstType, Pointer, Typedef,
     *      VolatileType), the equality decision is delegated to the referenced
     *      type's Equals() method.
     * \li Structured: all members of the above mention types are compared,
     *      but nested structs are ignored
     *
     * @param other the Instance object to compare this instance to
     * @return \c true if the two instances are considered to be equal, \c false
     * otherwise
     */
    bool Equals(const Instance& other) const;

    /**
     * Compares this struct or union Instance with \a other using the Equals()
     * method and returns a list of member names that are different.
     *
     * If this instance is not a struct or union or is not of the same type as
     * \a other, then a QStringList with just a single, empty string is
     * returned.
     *
     * If this instance equals \a other for all members, then an empty
     * QStringList is returned.
     *
     * @param other the Instance object to compare this instance to
     * @param recursive if \c true, this method recurses into nested structs
     *      or unions
     * @return If this Instance and \a other are structs or unions and
     * comparable, a list of member names that are different is returned.
     * Otherwise a list containing only one empty string is returned.
     */
    QStringList Differences(const Instance& other, bool recursive) const;

    /**
     * Use this method to check if an Instance object has a valid type.
     *
     * \warning This method does not tests the address of this instance. If you
     * additionally want to check whether this instance has a non-null address,
     * IsNull() instead.
     *
     * @return \c true if the instance has a valid type, \c false otherwise
     * \sa IsNull()
     */
    bool IsValid() const;

    /**
     * Checks if this instance has a non-null address \e and references a valid
     * type.  As JavaScript does not support 64-bit integers natively, one
     * would otherwise have to compare both the high and low values to test for
     * a null address:
     * \code
     * !AddressHigh() && !AddressLow()
     * \endcode
     * @return \c true if this instance has a non-null address as well as a
     * valid type, \c false otherwise
     * \sa IsValid()
     */
    bool IsNull() const;

    /**
     * Checks if this instance is accessible, meaning that its value can be read
     * from the memory dump through its virtual address.
     * @return \c true if the instance is accessible, \c false otherwise
     */
    bool IsAccessible() const;

    /**
     * Checks if this instance has a numeric type, that is, either any integer
     * type (\c char, \c short, \c int, \c long) or floating point type
     * (\c float, \c double).
     * @return \c true if this instance represents a numeric value, \c false
     * otherwise
     * \sa IsInteger(), IsReal()
     */
    bool IsNumber() const;

    /**
     * Checks if this instance has an integer type (\c char, \c short, \c int,
     * \c long).
     * @return \c true if this instance represents an integer value, \c false
     * otherwise
     * \sa IsNumber(), IsReal()
     */
    bool IsInteger() const;

    /**
     * Checks if this instance has a floating point type (\c float,
     * \c double).
     * @return \c true if this instance represents a floating point value,
     * \c false otherwise
     * \sa IsNumber(), IsInteger()
     */
    bool IsReal() const;

    /**
     * Changes the type of this instance to the type specified by its name
     * \a typeName. If that type cannot be found, a script exception is
     * injected.
     *
     * \note This does not change the virtual address of this instance.
     *
     * @param typeName the name of the type to change to
     */
    void ChangeType(const QString& typeName);

    /**
     * Changes the type of this instance to the type specified by its ID
     * \a typeId. If that type cannot be found, a script exception is injected.
     *
     * \note This does not change the virtual address of this instance.
     *
     * @param typeId the name of the type to change to
     */
    void ChangeType(int typeId);

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
     * Explicit representation of this instance as qint64. As JavaScript does
     * not support native 64-bit integers, the value is returned as a string.
     * The base is 10 by default and must be between 2 and 36. For bases other
     * than 10, the value is treated as an unsigned integer.
     * @param base numeric base to convert this string to
     * @return the value of this type as a qint64, converted to a string
     */
    QString toInt64(int base = 10) const;

    /**
     * Explicit representation of this instance as quint64. As JavaScript does
     * not support native 64-bit integers, the value is returned as a string.
     * The base is 10 by default and must be between 2 and 36. For bases other
     * than 10, the value is treated as an unsigned integer.
     * @param base numeric base to convert this string to
     * @return the value of this type as a quint64, converted to a string
     */
    QString toUInt64(int base = 10) const;

    /**
     * Explicit representation of this instance as quint64, returning only the
     * 32 most significant bits.
     * @return the 32 most significant bits of this type as a quint64
     */
    quint32 toUInt64High() const;

    /**
     * Explicit representation of this instance as quint64, returning only the
     * 32 least significant bits.
     * @return the 32 least significant bits of this type as a quint64
     */
    quint32 toUInt64Low() const;

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
     * @return a string representation of this instance, based on its type
     */
    QString toString() const;

    /**
     * Returns a toString() representation of this instance using the page
     * global directory (i. e., page table) specified as \a pgd for
     * virtual-to-physical address translation. This allows to read an instance
     * that is located in user-land address space.
     *
     * @param pgd the page global directory of the process this instance belongs
     * to, most likely the content of the \c CR3 register, as a hex-encoded string
     * @return the same as toString() but tries to access user-land memory,
     * if possible, using the page table specified as \a pgd
     */
    QString toStringUserLand(const QString &pgd) const;

private:
    inline Instance* thisInstance() const;
    inline void injectScriptError(const GenericException& e) const;
    inline void injectScriptError(const QString& msg) const;
};

#endif /* INSTANCEPROTOTYPE_H_ */
