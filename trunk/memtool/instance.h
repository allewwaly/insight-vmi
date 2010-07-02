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
#include <QSharedPointer>
#include <QMetaType>
#include <QVector>

class BaseType;
class VirtualMemory;
class Instance;

/// A reference counting pointer for Instance objects
typedef QSharedPointer<Instance> InstancePointer;

/// A list of Instance objects
typedef QVector<InstancePointer> InstancePointerVector;

/**
 * This class wraps a variable instance in a memory dump.
 */
class Instance: public QObject
{
	Q_OBJECT
	Q_PROPERTY(quint64 address READ address)
	Q_PROPERTY(QString name READ name)
	Q_PROPERTY(QStringList memberNames READ memberNames)
	Q_PROPERTY(QString typeName READ typeName)
	Q_PROPERTY(quint32 size READ size)

public:
	/**
	 * Constructor
	 * @param parent the parent QObject
	 */
	Instance(QObject* parent = 0);

	/**
	 * Constructor
	 * @param address the address of that instance
	 * @param type the underlying BaseType of this instance
	 * @param name the full name of this instance
	 * @param vmem the virtual memory device to read data from
	 */
	Instance(size_t address, const BaseType* type, const QString& name,
			VirtualMemory* vmem);

	/**
	 * Destructor
	 */
	virtual ~Instance();

	/**
	 * @return the virtual address of the variable in memory
	 */
	quint64 address() const;

	/**
	 * Use this function to retrieve the full name of this instance as it was
	 * found following the names and members of structs in dotted notation,
	 * i.e., \c init_task.children.next.pid.
	 * @return the full name of this instance
	 */
	QString name() const;

	/**
	 * Gives access to the names of all members if this instance.
	 * @return the names of all direct members of this instance
	 */
	const QStringList& memberNames() const;

	/**
	 * @return a vector of instances of all members
	 */
	InstancePointerVector members() const;

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

public slots:
	/**
	 * Checks if a member with the given name \a name exists in this instance.
	 * @param name the name of the member to find
	 * @return \c true if that member exists, \c false otherwise
	 */
	bool memberExists(const QString& name) const;

	/**
	 * Retrieves members (i.e., struct components) of this Instance, if they
	 * exist. You can check their existence with memberExists() or by iterating
	 * over the names returned by members().
	 *
	 * \note The returned pointer is wrapped in a QSharedPointer object for
	 * reference counting and automatic deletion of the created objects.
	 *
	 * @param name the name of the member to find
	 * @return a new Instance object if the member was found, \c null otherwise
	 */
	InstancePointer findMember(const QString& name) const;

	/**
	 * Retrieves the type ID of the member \a name.
	 * @param name the name of the member
	 * @return the ID of the type, if that member exists, \c 0 otherwise.
	 */
	int typeIdOfMember(const QString& name) const;

	/**
	 * @return a string representation of this instance
	 */
	QString toString() const;

private:
	static const QStringList _emtpyStringList;

	size_t _address;
	const BaseType* _type;
	QString _name;
    VirtualMemory* _vmem;
};

#endif /* INSTANCE_H_ */
