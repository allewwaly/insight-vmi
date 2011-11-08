#ifndef ASTEXPRESSIONEVALUATORTESTER_H
#define ASTEXPRESSIONEVALUATORTESTER_H

#include <QObject>
#include <QtTest>

class MemSpecs;
class SymFactory;
class ASTTypeEvaluator;
class ASTExpressionEvaluator;
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

private:
    MemSpecs* _specs;
    SymFactory* _factory;
    ASTTypeEvaluator* _eval;
    ASTExpressionEvaluator* _expr;
    AbstractSyntaxTree* _ast;
    ASTBuilder* _builder;
};

#endif // ASTEXPRESSIONEVALUATORTESTER_H
