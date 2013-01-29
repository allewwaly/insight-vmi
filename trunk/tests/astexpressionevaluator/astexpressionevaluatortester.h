#ifndef ASTEXPRESSIONEVALUATORTESTER_H
#define ASTEXPRESSIONEVALUATORTESTER_H

#include <QObject>
#include <QtTest>
#include <QByteArray>
#include <insight/astexpression.h>


class MemSpecs;
class KernelSymbols;
class KernelSourceTypeEvaluator;
class ASTExpressionTester;
class AbstractSyntaxTree;
class ASTBuilder;

class ASTExpressionEvaluatorTester : public QObject
{
    Q_OBJECT
public:
    explicit ASTExpressionEvaluatorTester(QObject *parent = 0);
    
private slots:
    /// Global init
    void initTestCase();
    /// Global cleanup
    void cleanupTestCase();

	/// Per test-case init
	void init();
	/// Per test-case cleanup
	void cleanup();

	void test_constants_func();
	void test_constants_func_data();

	void test_sign_extension_func();
	void test_sign_extension_func_data();

	void test_additive_func();
	void test_additive_func_data();

	void test_multiplicative_func();
	void test_multiplicative_func_data();

	void test_logical_func();
	void test_logical_func_data();

	void test_bitwise_func();
	void test_bitwise_func_data();

	void test_equality_func();
	void test_equality_func_data();

	void test_relational_func();
	void test_relational_func_data();

	void test_shift_func();
	void test_shift_func_data();

	void test_unary_func();
	void test_unary_func_data();

	void test_builtins_func();
	void test_builtins_func_data();

private:
    template <class T>
    ExprResultSizes exprSize(T x) const;

    const MemSpecs* _specs;
    KernelSymbols* _symbols;
    KernelSourceTypeEvaluator* _eval;
    ASTExpressionTester* _tester;
    AbstractSyntaxTree* _ast;
    ASTBuilder* _builder;
    QByteArray _ascii;
};

#endif // ASTEXPRESSIONEVALUATORTESTER_H
