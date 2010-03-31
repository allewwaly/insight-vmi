/*
 * Instance.h
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include <sys/types.h>
#include <QVariant>

class BaseType;

class Instance
{
public:
	Instance(const QString& name, const BaseType* type, size_t offset);

	template<class T>
	QVariant toVariant() const;

	QString toString() const;

protected:
	QString _name;
	const BaseType* _type;
	size_t _offset;
};

#endif /* INSTANCE_H_ */
