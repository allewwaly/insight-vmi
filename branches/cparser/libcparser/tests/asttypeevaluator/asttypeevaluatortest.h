/*
 * asttypeevaluatortest.h
 *
 *  Created on: 17.08.2011
 *      Author: chrschn
 */

#ifndef ASTTYPEEVALUATORTEST_H_
#define ASTTYPEEVALUATORTEST_H_

#include <C.h>
#include <QObject>
#include <QByteArray>

class AbstractSyntaxTree;
class ASTBuilder;
class ASTTypeEvaluatorTester;

class ASTTypeEvaluatorTest: public QObject
{
	Q_OBJECT
public:
	ASTTypeEvaluatorTest(QObject* parent = 0);
	virtual ~ASTTypeEvaluatorTest();

private slots:
	void init();
	void cleanup();

	void allTests();
	void allTests_data();

private:
	AbstractSyntaxTree* _ast;
	ASTBuilder* _builder;
	ASTTypeEvaluatorTester* _tester;
	QByteArray _ascii;
};

#endif /* ASTTYPEEVALUATORTEST_H_ */
