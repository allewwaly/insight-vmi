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

class SymFactory;

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
class InstancePrototype : public QObject, protected QScriptable
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent parent object, defaults to 0
     */
    InstancePrototype(const SymFactory* factory, QObject *parent = 0);

    /**
     * Destructor
     */
    virtual ~InstancePrototype();

    /**
     * Returns the used knowledge sources when resolving members.
     * \sa setKnowledgeSources()
     */
    inline KnowledgeSources knowledgeSources() const
    {
        return _knowSrc;
    }

    /**
     * Sets the used knowledge sources when resolving members.
     * @param src knowlege sources
     * \sa knowledgeSources()
     */
    inline void setKnowledgeSources(KnowledgeSources src)
    {
        _knowSrc = src;
    }

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
     * Adds \a offset to the virtual address. The address must be given as hex
     * encoded string, which may optionally start with "0x".
     * @param offset the hex encoded offset to add
     * \sa Address(), SetAddress()
     */
    void AddToAddress(QString offset);

    /**
     * Treats this Instance as an array and returns a new instance
     * at index \a index. The behavior depends on the Type():
     *
     * \li For pointers of type <tt>T*</tt>, the pointer is dereferenced and
     *    \a index * <tt>sizeof(T)</tt> is added to the resulting address. The
     *    resulting type is <tt>T</tt>. If the pointer cannot be dereferenced,
     *    either because it is of type <tt>void*</tt> or the address is not
     *    accessible, an empty Instance is returned.
     * \li For arrays of type <tt>T[]</tt>, \a index * <tt>sizeof(T)</tt> is
     *    added to the current address. The resulting type is <tt>T</tt>.
     * \li For any other type <tt>T</tt>, only \a index * <tt>sizeof(T)</tt> is
     *    added to the current address. The type <tt>T</tt> remains unchanged.
     *
     * \warning The ArrayLength() parameter is never checked against \a index!
     *
     * @param index array index to access
     * @return a new Instance as described above
     *\sa ArrayLength()
     */
    Instance ArrayElem(int index) const;

    /**
     * If this Instance represents an Array with a defined length, this length
     * os returned by this function, otherwise the return value is -1.
     * @return array length field as described above
     * \sa ArrayElem()
     */
    int ArrayLength() const;

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
     * Returns the name of member no. \a index. Calling "inst.MemberName(i)" is
     * much more efficient than calling "inst.Member(i).Name()", since it does
     * not construct an intermediate Instance object.
     * @param index index into the member list
     * @return name of member \a index
     * \sa MemberCount(), Members(), MemberNames()
     */
    QString MemberName(int index) const;

    /**
     * Gives access to the names of all members if this instance.
     * @return a list of the names of all direct members of this instance
     * \sa Members(), MemberExists()
     */
    QStringList MemberNames() const;

    /**
     * Gives access to all members if this instance. If a member has exactly
     * one candidate type, this type will be used instead of the originally
     * declared type of the mamber. To have the declared types instead, set
     * \a declaredTypes to \c true.
     * @param declaredTypes selects if candidate types or declared types should
     * be used, where applicable
     * @return a list of instances of all members
     * \sa MemberNames(), Member()
     */
    InstanceList Members(bool declaredTypes = false) const;

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

    uint TypeHash() const;

    /**
     * Returns the value of \c sizeof(type) for this instance's type.
     * @return the size of this instance's type
     */
    quint32 Size() const;

    /**
     * If this instance represents a pointer, a dereferenced instance is
     * returned, otherwise this instance is returned as-is. For multi-level
     * pointers, the number of derefenciations can be limited with
     * \a maxPtrDeref. The default value of -1 dereferences all pointer levels.
     * @param maxPtrDeref max. number of pointer dereferenciations, use -1 for
     * infinity
     * @return a dereferenced version of this instance
     * \sa Type()
     */
    Instance Dereference(int maxPtrDeref = -1) const;


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
     * @param index the index of the member
     * @return offset of member \a name within the struct, or -1 if no such
     * member exists or if this instance is no struct or union
     */
    int MemberOffset(int index) const;

    /**
     * Calculates the offset of a member within a struct, if this is a struct
     * or union.
     * @param name the name of the member
     * @return offset of member \a name within the struct, or -1 if no such
     * member exists or if this instance is no struct or union
     */
    int MemberOffset(const QString& name) const;

    /**
     * Retrieves the number of members of this instance.
     * @return the number of members of this instance.
     */
    int MemberCount() const;

    /**
     * Retrieves the member at index \a index of this Instance, if it exists.
     * You can check for the number of members with MemberCount().
     *
     * @param index the index of the member
     * @param declaredType if set to \c false (the default), all enabled
     *  knowledge sources will be considered to resolve this member and all
     *  pointer types will be dereferenced;
     *  if set to \c true, the originally declared type for this member will be
     *  returned and only lexical types will be dereferenced (but no pointers)
     * @return a new Instance object if the member exists, or an empty
     * object otherwise
     * \sa MemberCount()
     */
    Instance Member(int index, bool declaredType = false) const;

    /**
     * Retrieves a member of this Instance, if it exists. If this struct or
     * union has no member \a name, all anonymous nested structs or unions are
     * searched as well. This is conforming to the C standard.
     *
     * \note Make sure to check IsValid() on the returned object to see if it is
     * valid or not.
     *
     * @param name the name of the member to find
     * @param declaredType if set to \c false (the default), all enabled
     *  knowledge sources will be considered to resolve this member and all
     *  pointer types will be dereferenced;
     *  if set to \c true, the originally declared type for this member will be
     *  returned and only lexical types will be dereferenced (but no pointers)
     * @return a new Instance object if the member was found, or an empty
     * object otherwise
     * \sa MemberExists(), MemberNames(), IsValid()
     */
    Instance Member(const QString& name, bool declaredType = false) const;

    /**
     * Retrieves the type ID of the member \a name.
     * @param name the name of the member
     * @param declaredType if set to \c false (the default), all enabled
     *  knowledge sources will be considered to resolve this member and all
     *  pointer types will be dereferenced;
     *  if set to \c true, the originally declared type for this member will be
     *  returned and only lexical types will be dereferenced (but no pointers)
     * @return the ID of the type, if that member exists, \c 0 otherwise.
     */
    int TypeIdOfMember(const QString& name, bool declaredType = false) const;

    /**
     * Returns the number of candidate types for a particular member.
     * @param name the name of the member
     * @return 0 if only the originally declared type is available, otherwise
     * the number of alternative candidate types
     */
    int MemberCandidatesCount(const QString& name) const;

    /**
     * Returns the number of candidate types for a particular member.
     * @param index the index of the member
     * @return 0 if only the originally declared type is available, otherwise
     * the number of alternative candidate types
     */
    int MemberCandidatesCount(int index) const;

    /**
     * Retrieves the candidate type no. \a cndtIndex for member with index
     * \a mbrIndex. You can check for the number of members with MemberCount()
     * and the number of candidate types for a particular member with
     * MemberCandidatesCount().
     * @param mbrIndex index of the member
     * @param cndtIndex index of the candidate type for that member
     * @return a new Instance object for the member with the selected candidate
     * type, if such a member and candiate exists, or an empty object otherwise
     * \sa MemberCount(), MemberCandidatesCount()
     */
    Instance MemberCandidate(int mbrIndex, int cndtIndex) const;

    /**
     * Retrieves the candidate type no. \a cndtIndex for member \a name.
     * You can check for the number of candidate types for a particular member
     * with MemberCandidatesCount().
     * @param name the name of the member
     * @param cndtIndex index of the candidate type for that member
     * @return a new Instance object if the member \a name with the selected
     * candidate type exists, or an empty object otherwise
     * \sa MemberCount(), MemberCandidatesCount()
     */
    Instance MemberCandidate(const QString& name, int cndtIndex) const;

    /**
     * Checks if this Instance is compatible with the expression that needs to
     * be evaluated for candidate type with index \a cndtIndex of member at
     * index \a mbrIndex.
     * @param mbrIndex the member index
     * @param cndtIndex index of the candidate type for that member
     * @return \c true if this Instance is compatible with the expression,
     * \c false otherwise
     */
    bool MemberCandidateCompatible(int mbrIndex, int cndtIndex) const;

    /**
     * Checks if this Instance is compatible with the expression that needs to
     * be evaluated for candidate type with index \a cndtIndex of member
     * \a name.
     * @param name the name of the member
     * @param cndtIndex index of the candidate type for that member
     * @return \c true if this Instance is compatible with the expression,
     * \c false otherwise
     */
    bool MemberCandidateCompatible(const QString& name, int cndtIndex) const;

    /**
     * Calculates the virtual address of a member, if this is a struct or union.
     * @param index index into the member list
     * @param declaredType selects if the candidate type (if it exists) or the
     * declared types should be used, defaults to \c false
     * @return the virtual address of member \a index as a string, or "0" if
     * this is no struct or union
     */
    QString MemberAddress(int index, bool declaredType = false) const;

    /**
     * Calculates the virtual address of member \a name, if this is a struct or
     * union.
     * @param name the name of the member
     * @param declaredType selects if the candidate type (if it exists) or the
     * declared types should be used, defaults to \c false
     * @return the virtual address of member \a name as a string, or "0" if
     * this is no struct or union
     */
    QString MemberAddress(const QString& name, bool declaredType = false) const;

    /**
     * This method always returns 4 on 32-bit kernels and 8 on 64-bit kernels.
     * @return the size of pointers for this architecture, in bytes
     */
    int SizeofPointer() const;

    /**
     * This method returns the storage size of type <tt>long int</tt> on the
     * guest architecture.
     * @return the size of <tt>long int</tt>, in bytes
     */
    int SizeofLong() const;

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
     * Checks if this instance has a non-null address.
     * As JavaScript does not support 64-bit integers natively, one
     * would otherwise have to compare both the high and low values to test for
     * a null address:
     * \code
     * !AddressHigh() && !AddressLow()
     * \endcode
     * @return \c true if this instance has a non-null address, \c false
     * otherwise
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
     * Returns the name of the kernel or module file this symbol was read from.
     */
    QString OrigFileName() const;

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
     * \sa toUInt64()
     */
    QString toInt64(int base = 10) const;

    /**
     * Explicit representation of this instance as quint64. As JavaScript does
     * not support native 64-bit integers, the value is returned as a string.
     * The base is 10 by default and must be between 2 and 36. For bases other
     * than 10, the value is treated as an unsigned integer.
     * @param base numeric base to convert this string to
     * @return the value of this type as a quint64, converted to a string
     * \sa toUInt64Low(), toUInt64High()
     */
    QString toUInt64(int base = 10) const;

    /**
     * Explicit representation of this instance as quint64, returning only the
     * 32 most significant bits.
     * @return the 32 most significant bits of this type as a quint64
     * \sa toUInt64Low()
     */
    quint32 toUInt64High() const;

    /**
     * Explicit representation of this instance as quint64, returning only the
     * 32 least significant bits.
     * @return the 32 least significant bits of this type as a quint64
     * \sa toUInt64High()
     */
    quint32 toUInt64Low() const;

    /**
     * Explicit representation of this instance as \c long value.  As JavaScript
     * does not support native 64-bit integers, the value is always returned as
     * a string, even for platforms having a 32-bit \c long type.
     * The base is 10 by default and must be between 2 and 36. For bases other
     * than 10, the value is treated as an unsigned integer.
     * @param base numeric base to convert this string to
     * @return the value of this type as a qint64, converted to a string
     * \sa toULong(), SizeofLong()
     */
    QString toLong(int base = 10) const;

    /**
     * Explicit representation of this instance as <tt>unsigned long</tt> value.
     * Since JavaScript does not support native 64-bit integers, the value is
     * always returned as a string, even for platforms having a 32-bit \c long
     * type.
     * The base is 10 by default and must be between 2 and 36. For bases other
     * than 10, the value is treated as an unsigned integer.
     * @param base numeric base to convert this string to
     * @return the value of this type as a qint64, converted to a string
     * \sa toLong(), SizeofLong()
     */
    QString toULong(int base = 10) const;

    /**
     * Explicit representation of this instance as pointer. As JavaScript does
     * not support native 64-bit integers, the value is always returned as a
     * string, even for 32-bit systems.
     * The base is 16 by default and must be between 2 and 36.
     * @param base numeric base to convert this string to
     * @return the value of this type as a pointer, converted to a string
     * \sa SizeofPointer()
     */
    QString toPointer(int base = 16) const;

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

//    /**
//     * Returns a toString() representation of this instance using the page
//     * global directory (that is, page table) specified as \a pgd for
//     * virtual-to-physical address translation. This allows to read an instance
//     * that is located in user-land address space.
//     *
//     * @param pgd the page global directory of the process this instance belongs
//     * to, most likely the content of the \c CR3 register, as a hex-encoded string
//     * @return the same as toString() but tries to access user-land memory,
//     * if possible, using the page table specified as \a pgd
//     */
//    QString toStringUserLand(const QString &pgd) const;

private:
    inline Instance* thisInstance() const;
    inline void injectScriptError(const GenericException& e) const;
    inline void injectScriptError(const QString& msg) const;
    KnowledgeSources _knowSrc;
    const SymFactory* _factory;
};

#endif /* INSTANCEPROTOTYPE_H_ */
