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
#include <QTextStream>

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

	void test_basic_func();
	void test_basic_func_data();

	void test_listHeadEqual_func();
	void test_listHeadEqual_func_data();

	void test_postfixOperators_func();
	void test_postfixOperators_func_data();

	void test_basicTypeCasts_func();
	void test_basicTypeCasts_func_data();

	void test_functionCalls_func();
	void test_functionCalls_func_data();

	void test_basicTypeChanges_func();
	void test_basicTypeChanges_func_data();

	void test_basicCastingChanges_func();
	void test_basicCastingChanges_func_data();

	void test_ignoredTypeChanges_func();
	void test_ignoredTypeChanges_func_data();

	void test_structInitializers_func();
	void test_structInitializers_func_data();

	void test_designatedInitializers_func();
	void test_designatedInitializers_func_data();

	void test_arrayInitializers_func();
	void test_arrayInitializers_func_data();

	void test_returnValues_func();
	void test_returnValues_func_data();

	void test_functionDefsDecls_func();
	void test_functionDefsDecls_func_data();

	void test_functionPointers_func();
	void test_functionPointers_func_data();

	void test_functionPointerTypedefs_func();
	void test_functionPointerTypedefs_func_data();

	void test_functionPointerParams_func();
	void test_functionPointerParams_func_data();

	void test_functionPointerInvocations_func();
	void test_functionPointerInvocations_func_data();

	void test_funcPtrTypdefPtrs_func();
	void test_funcPtrTypdefPtrs_func_data();

	void test_funcPtrTypdefPtrInvocations_func();
	void test_funcPtrTypdefPtrInvocations_func_data();

	void test_logicalExpressions_func();
	void test_logicalExpressions_func_data();

	void test_castExpressions_func();
	void test_castExpressions_func_data();

	void test_conditionalExpressions_func();
	void test_conditionalExpressions_func_data();

	void test_pointerDerefByArrayOperator_func();
	void test_pointerDerefByArrayOperator_func_data();

	void test_compoundBraces_func();
	void test_compoundBraces_func_data();

	void test_transitiveEvaluation_func();
	void test_transitiveEvaluation_func_data();

	void test_transitiveFunctions_func();
	void test_transitiveFunctions_func_data();

	void test_transitiveIndirectPtrs_func();
	void test_transitiveIndirectPtrs_func_data();

	void test_transitivePtrSensitivity_func();
	void test_transitivePtrSensitivity_func_data();

	void test_transitiveFieldSensitivity_func();
	void test_transitiveFieldSensitivity_func_data();

	void test_transitivePostfixSuffixes_func();
	void test_transitivePostfixSuffixes_func_data();

	void test_transitiveNestedPrimary_func();
	void test_transitiveNestedPrimary_func_data();

private:
	AbstractSyntaxTree* _ast;
	ASTBuilder* _builder;
	ASTTypeEvaluatorTester* _tester;
	QByteArray _ascii;
	QString _err_str;
	QTextStream _err;
};

#endif /* ASTTYPEEVALUATORTEST_H_ */
