#ifndef ASTEXPRESSIONEVALUATORTESTER_H
#define ASTEXPRESSIONEVALUATORTESTER_H

#include <QObject>
#include <QtTest>
#include <QByteArray>

class MemSpecs;
class SymFactory;
class ASTTypeEvaluator;
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

	void test_arithmetic_func();
	void test_arithmetic_func_data();

	void test_sign_extension_func();
	void test_sign_extension_func_data();

private:
    MemSpecs* _specs;
    SymFactory* _factory;
    ASTTypeEvaluator* _eval;
    ASTExpressionTester* _tester;
    AbstractSyntaxTree* _ast;
    ASTBuilder* _builder;
    QByteArray _ascii;
};

#endif // ASTEXPRESSIONEVALUATORTESTER_H
