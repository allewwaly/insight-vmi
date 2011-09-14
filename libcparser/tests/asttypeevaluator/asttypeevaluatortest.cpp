/*
 * asttypeevaluatortest.cpp
 *
 *  Created on: 17.08.2011
 *      Author: chrschn
 */

#include "asttypeevaluatortest.h"
#include <asttypeevaluator.h>
#include <abstractsyntaxtree.h>
#include <astbuilder.h>
#include <debug.h>
#include <genericexception.h>
#include <QtTest>
#include <iostream>


class ASTTypeEvaluatorTester: public ASTTypeEvaluator
{
public:
	ASTTypeEvaluatorTester(AbstractSyntaxTree* ast)
		: ASTTypeEvaluator(ast, 4) { reset(); }

	void reset()
	{
		typeChanged = false;
		symbolName.clear();
		ctxType.clear();
		ctxMembers.clear();
		targetType.clear();
	}

	bool typeChanged;
	QString symbolName;
	QString ctxType;
	QString ctxMembers;
	QString targetType;

protected:
    virtual void primaryExpressionTypeChange(const ASTNode* srcNode,
    		const ASTSymbol& symbol, const ASTType* ctxType, const ASTNode* ctxNode,
    		const QStringList& ctxMembers, const ASTNode* targetNode,
    		const ASTType* targetType)
    {
    	this->typeChanged = true;
    	this->symbolName = symbol.name();
    	this->ctxType = ctxType->toString();
    	this->ctxMembers = ctxMembers.join(".");
    	this->targetType = targetType->toString();
    }
};

#define safe_delete(x) do { if (x) delete x; (x) = 0; } while (0)


ASTTypeEvaluatorTest::ASTTypeEvaluatorTest(QObject* parent)
	: QObject(parent), _ast(0), _builder(0)
{
}


ASTTypeEvaluatorTest::~ASTTypeEvaluatorTest()
{
	cleanup();
}


void ASTTypeEvaluatorTest::init()
{
    _ast = new AbstractSyntaxTree();
    _builder = new ASTBuilder(_ast);
    _tester = new ASTTypeEvaluatorTester(_ast);
}


void ASTTypeEvaluatorTest::cleanup()
{
	if (!_ascii.isEmpty()) {
		QString asciiStr = QString::fromAscii(_ascii.constData(), _ascii.size());
		std::cerr << "--------[Failed program]--------" << std::endl
				<< asciiStr.toStdString() << std::endl
				<< "--------[/Failed program]--------" << std::endl;
		_ascii.clear();
	}
	safe_delete(_tester);
	safe_delete(_builder);
	safe_delete(_ast);
}

#define DEF_LIST_HEAD \
		"struct list_head {\n" \
		"    struct list_head *next, *prev;\n" \
		"};\n" \
		"\n" \

#define DEF_MODULES \
		"struct module {\n" \
		"    int foo;\n" \
		"    struct list_head list;\n" \
		"    struct list_head* plist;\n" \
		"};\n"\
		"\n" \
		"struct list_head modules;\n" \
		"\n"

#define DEF_FUNCS \
		"extern struct list_head* func();\n" \
		"struct list_head* (*func_ptr)();\n" \
		"\n"

#define DEF_HEADS \
		"struct list_head* heads[4];\n" \
		"\n"

#define DEF_MAIN_BEGIN \
		"void main(struct module* m)\n" \
		"{\n" \
		"    struct list_head* h;\n" \
		"    struct module* m;\n" \
		"\n"

#define DEF_MAIN_END \
		"}\n" \
		"\n"

#define DEF_HEADER DEF_LIST_HEAD DEF_MODULES DEF_FUNCS DEF_HEADS


void ASTTypeEvaluatorTest::allTests_data()
{
	QTest::addColumn<QString>("globalCode");
	QTest::addColumn<QString>("localCode");
	QTest::addColumn<bool>("typeChanged");
	QTest::addColumn<QString>("symbolName");
	QTest::addColumn<QString>("ctxType");
	QTest::addColumn<QString>("ctxMembers");
	QTest::addColumn<QString>("targetType");
	QTest::addColumn<QString>("exceptionMsg");

	// Very basic type equality and changes
	QTest::newRow("basicInitiEqual") << "" << "void* q; void *p = q;" << false;
	QTest::newRow("basicAssignmentEqual") << "" << "void* q, *p; p = q;" << false;
	QTest::newRow("basicInitNoIdentifier") << "" << "void* q = (1+2)*3;" << false;
	QTest::newRow("basicInitChange1") << "" << "int i; void *p = i;" << true
		<< "i" << "Int32" << "" << "Pointer->Void";
	QTest::newRow("basicInitChange2") << "" << "void *p; int i = p;" << true
		<< "p" << "Pointer->Void" << "" << "Int32";
	QTest::newRow("basicAssignmentChange1") << "" << "int i; void *p; p = i;" << true
		<< "i" << "Int32" << "" << "Pointer->Void";
	QTest::newRow("basicAssignmentChange2") << "" << "void *p; int i; i = p;" << true
		<< "p" << "Pointer->Void" << "" << "Int32";

	// Various equality checks for list_head
    QTest::newRow("listHeadEqual1") << "" << "h = heads[0];" << false;
    QTest::newRow("listHeadEqual2") << "" << "heads[0] = h;" << false;
    QTest::newRow("listHeadEqual3") << "" << "h = *heads;" << false;
    QTest::newRow("listHeadEqual4") << "" << "*heads = h;" << false;
    QTest::newRow("listHeadEqual5") << "" << "heads = &h;" << false;
    QTest::newRow("listHeadEqual6") << "" << "m->list.next = h;" << false;
    QTest::newRow("listHeadEqual7") << "" << "h = m->list.next;" << false;

    // Array, arrow and star operator, all in the mix
    QTest::newRow("arrayArrowStar1") << "" << "h = m->plist->next;" << false;
    QTest::newRow("arrayArrowStar2") << "" << "h = m[0].plist->next;" << false;
    QTest::newRow("arrayArrowStar3") << "" << "h = m->plist[0].next;" << false;
    QTest::newRow("arrayArrowStar4") << "" << "h = m[0].plist[0].next;" << false;
    QTest::newRow("arrayArrowStar5") << "" << "h = (*m).plist->next;" << false;
    QTest::newRow("arrayArrowStar6") << "" << "h = (*m->plist).next;" << false;
    QTest::newRow("arrayArrowStar7") << "" << "h = (*m).plist[0].next;" << false;
    QTest::newRow("arrayArrowStar8") << "" << "h = (*m[0].plist).next;" << false;

    QTest::newRow("arrayArrowStar9") << "" << "*h = *m->plist->next;" << false;
    QTest::newRow("arrayArrowStar10") << "" << "*h = *m[0].plist->next;" << false;
    QTest::newRow("arrayArrowStar11") << "" << "*h = *m->plist[0].next;" << false;
    QTest::newRow("arrayArrowStar12") << "" << "*h = *m[0].plist[0].next;" << false;
    QTest::newRow("arrayArrowStar13") << "" << "*h = *(*m).plist->next;" << false;
    QTest::newRow("arrayArrowStar14") << "" << "*h = *(*m->plist).next;" << false;
    QTest::newRow("arrayArrowStar15") << "" << "*h = *(*m).plist[0].next;" << false;
    QTest::newRow("arrayArrowStar16") << "" << "*h = *(*m[0].plist).next;" << false;

    QTest::newRow("arrayArrowStar17") << "" << "*h = m->plist->next[0];" << false;
    QTest::newRow("arrayArrowStar18") << "" << "*h = m[0].plist->next[0];" << false;
    QTest::newRow("arrayArrowStar19") << "" << "*h = m->plist[0].next[0];" << false;
    QTest::newRow("arrayArrowStar20") << "" << "*h = m[0].plist[0].next[0];" << false;
    QTest::newRow("arrayArrowStar21") << "" << "*h = (*m).plist->next[0];" << false;
    QTest::newRow("arrayArrowStar22") << "" << "*h = (*m->plist).next[0];" << false;
    QTest::newRow("arrayArrowStar23") << "" << "*h = (*m).plist[0].next[0];" << false;
    QTest::newRow("arrayArrowStar24") << "" << "*h = (*m[0].plist).next[0];" << false;

    // Type casts that lead to equal types
    QTest::newRow("castingEqual1") << "" << "h = (struct list_head*)m->list.next;" << false;
    QTest::newRow("castingEqual2") << "" << "h = (struct list_head*)((struct module*)m)->list.next;" << false;
    QTest::newRow("castingEqual3") << "" << "h = (struct list_head*)(*((struct module*)m)).list.next;" << false;

    // Function calls
    QTest::newRow("funcCalls1") << "" << "h = func();" << false;
    QTest::newRow("funcCalls2") << "" << "*h = *func();" << false;
    QTest::newRow("funcCalls3") << "" << "h = func_ptr();" << false;
    QTest::newRow("funcCalls4") << "" << "*h = *func_ptr();" << false;

    // Basic type changes
    QTest::newRow("basicChanges1") << "" << "h = m;" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("basicChanges2") << "" << "m = h;" << true
        << "h" << "Pointer->Struct(list_head)" << "" << "Pointer->Struct(module)";
    QTest::newRow("basicChanges2") << "" << "m = h->next;" << true
        << "h" << "Struct(list_head)" << "next" << "Pointer->Struct(module)";
    QTest::newRow("basicChanges3") << "" << "m = h->next->prev;" << true
        << "h" << "Struct(list_head)" << "prev" << "Pointer->Struct(module)";

    // Casting changes
    QTest::newRow("castingChanges1") << "" << "m = (struct module*)h->next;" << true
        << "h" << "Struct(list_head)" << "next" << "Pointer->Struct(module)";
    QTest::newRow("castingChanges2") << "" << "m = (struct list_head*)h->next;" << true
        << "h" << "Struct(list_head)" << "next" << "Pointer->Struct(module)";
    QTest::newRow("castingChanges3") << "" << "m = (void*)h->next;" << true
        << "h" << "Struct(list_head)" << "next" << "Pointer->Struct(module)";

    // list_head-like casting changes
    QTest::newRow("listHeadCasts1") << "" << "m = (struct module*)(((char*)modules.next) - __builtin_offsetof(struct module, list));" << true
        << "modules" << "Struct(list_head)" << "next" << "Pointer->Struct(module)";

    // Ignore type changes in certain cases
    QTest::newRow("ignoreCastInBuiltin") << "" << "int i = sizeof((void*)m);" << false;
    QTest::newRow("ignoreNoAssignment") << "" << "(void*)m;" << false;
    QTest::newRow("ignoreNoIdentifier") << "" << "h = (void*)(1+2);" << false;

    // Struct initializers
    QTest::newRow("structInitEqual1") << "" << "struct list_head foo = { h->next, h->prev };" << false;
    QTest::newRow("structInitEqual2") << "" << "struct list_head foo = { m->list.next, m->list.prev };" << false;
    QTest::newRow("structInitEqual3") << "" << "struct list_head foo = { .prev = h, .next = h };" << false;
    QTest::newRow("structInitEqual4") << "" << "struct list_head foo = { &m->list, &m->list };" << false;
    QTest::newRow("structInitEqual5") << "" << "struct module foo = { .foo = 0, .list = { h, h } };" << false;
    QTest::newRow("structInitEqual5") << "" << "struct module foo = { 0, { h, h } };" << false;

    QTest::newRow("structInitChanges1") << "" << "struct list_head foo = { .prev = m, .next = h };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("structInitChanges2") << "" << "struct list_head foo = { .prev = (struct list_head*)m, .next = h };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("structInitChanges3") << "" << "struct list_head foo = { .prev = (struct list_head*)m->foo, .next = h };" << true
        << "m" << "Struct(module)" << "foo" << "Pointer->Struct(list_head)";
    QTest::newRow("structInitChanges4") << "" << "struct module foo = { m, { h, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Int32";
    QTest::newRow("structInitChanges5") << "" << "struct module foo = { 0, { m, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("structInitChanges6") << "" << "struct module foo = { .foo = m, .list = { h, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Int32";
    QTest::newRow("structInitChanges7") << "" << "struct module foo = { .foo = 0, .list = { m, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";

    // Designated struct initializers
    QTest::newRow("designatedInitEqual1") << "struct list_head foo;\n" << "foo = (struct list_head){ h->next, h->prev };" << false;
    QTest::newRow("designatedInitEqual2") << "struct list_head foo;\n" << "foo = (struct list_head){ m->list.next, m->list.prev };" << false;
    QTest::newRow("designatedInitEqual3") << "struct list_head foo;\n" << "foo = (struct list_head){ .prev = h, .next = h };" << false;
    QTest::newRow("designatedInitEqual4") << "struct list_head foo;\n" << "foo = (struct list_head){ &m->list, &m->list };" << false;
    QTest::newRow("designatedInitEqual5") << "struct module foo;\n" << "foo = (struct module){ .foo = 0, .list = { h, h } };" << false;
    QTest::newRow("designatedInitEqual6") << "struct module foo;\n" << "foo = (struct module){ 0, { h, h } };" << false;

    QTest::newRow("designatedInitChanges1") << "struct list_head foo;\n" << "struct list_head foo = { .prev = m, .next = h };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("designatedInitChanges2") << "struct list_head foo;\n" << "struct list_head foo = { .prev = (struct list_head*)m, .next = h };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("designatedInitChanges3") << "struct list_head foo;\n" << "struct list_head foo = { .prev = (struct list_head*)m->foo, .next = h };" << true
        << "m" << "Struct(module)" << "foo" << "Pointer->Struct(list_head)";
    QTest::newRow("designatedInitChanges4") << "struct module foo;\n" << "foo = (struct module){ m, { h, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Int32";
    QTest::newRow("designatedInitChanges5") << "struct module foo;\n" << "foo = (struct module){ 0, { m, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("designatedInitChanges6") << "struct module foo;\n" << "foo = (struct module){ h, { 0, 0 } };" << true
        << "h" << "Pointer->Struct(list_head)" << "" << "Int32";
    QTest::newRow("designatedInitChanges7") << "struct module foo;\n" << "foo = (struct module){ .foo = m, .list = { h, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Int32";
    QTest::newRow("designatedInitChanges8") << "struct module foo;\n" << "foo = (struct module){ .foo = 0, .list = { m, h } };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("designatedInitChanges9") << "struct module foo;\n" << "foo = (struct module){ .foo = h, .list = { 0, 0 } };" << true
        << "h" << "Pointer->Struct(list_head)" << "" << "Int32";

    // Array initializers
    QTest::newRow("arrayInitEqual1") << "" << "struct list_head a[2] = { {h, h}, {h, h} };" << false;
    QTest::newRow("arrayInitEqual2") << "" << "struct list_head a[2] = { [0] = {h, h} };" << false;
    QTest::newRow("arrayInitEqual3") << "" << "struct list_head a[2] = { [0] = { .prev = h, .next = h } };" << false;
    QTest::newRow("arrayInitEqual4") << "" << "struct module* a[2] = { m, m };" << false;
    QTest::newRow("arrayInitEqual5") << "" << "struct module* a[2] = { [0] = m };" << false;

    QTest::newRow("arrayInitChange1") << "" << "struct list_head a[2] = { {h, m}, {h, h} };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("arrayInitChange2") << "" << "struct list_head a[2] = { {h, h}, {h, m} };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("arrayInitChange3") << "" << "struct list_head a[2] = { [0] = {h, m} };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("arrayInitChange4") << "" << "struct list_head a[2] = { [0] = { .prev = m, .next = h} };" << true
        << "m" << "Pointer->Struct(module)" << "" << "Pointer->Struct(list_head)";
    QTest::newRow("arrayInitChange5") << "" << "struct module* a[2] = { h->next, m };" << true
        << "h" << "Struct(list_head)" << "next" << "Pointer->Struct(module)";
    QTest::newRow("arrayInitChange6") << "" << "struct module* a[2] = { h->prev, m };" << true
        << "h" << "Struct(list_head)" << "prev" << "Pointer->Struct(module)";
    QTest::newRow("arrayInitChange7") << "" << "struct module* a[2] = { [0] = h->prev };" << true
        << "h" << "Struct(list_head)" << "prev" << "Pointer->Struct(module)";

    // Return values
    QTest::newRow("returnNoValue") << "struct module* foo() { return; }" << "" << false;
    QTest::newRow("returnNullValue") << "struct module* foo() { return 0; }" << "" << false;
    QTest::newRow("returnSameValue") << "struct module* foo(struct module* p) { return p; }" << "" << false;
    QTest::newRow("returnDifferentValue") << "struct module* foo(void* p) { return p; }" << "" << true
        << "p" << "Pointer->Void" << "" << "Pointer->Struct(module)";
    QTest::newRow("returnDifferentValue") << "struct module* foo(long i) { return i; }" << "" << true
        << "i" << "Int32" << "" << "Pointer->Struct(module)";

    // Function definitions
    QTest::newRow("funcPtr1") << "int foo() { return 0; }" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr2") << "int* foo() { return 0; }" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr3") << "int** foo() { return 0; }" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Function declarations
    QTest::newRow("funcPtr4") << "int foo();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr5") << "int* foo();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr6") << "int** foo();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Function pointers
    QTest::newRow("funcPtr7") << "int   (  foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr8") << "int*  (  foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr9") << "int** (  foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr10") << "int   (* foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr11") << "int*  (* foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr12") << "int** (* foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr13") << "int   (**foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr14") << "int*  (**foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr15") << "int** (**foo)();" << "void* p = foo;" << true
        << "foo" << "FuncPointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Function pointer typedefs
    QTest::newRow("funcPtr16") << "typedef int   (  foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr17") << "typedef int*  (  foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr18") << "typedef int** (  foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr19") << "typedef int   (* foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr20") << "typedef int*  (* foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr21") << "typedef int** (* foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr22") << "typedef int   (**foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr23") << "typedef int*  (**foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr24") << "typedef int** (**foodef)(); foodef foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Function pointer parameters
    QTest::newRow("funcPtr25") << "void func(int   (  foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr26") << "void func(int*  (  foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr27") << "void func(int** (  foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr28") << "void func(int   (* foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr29") << "void func(int*  (* foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr30") << "void func(int** (* foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr31") << "void func(int   (**foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr32") << "void func(int*  (**foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr33") << "void func(int** (**foo)()) { void* p = foo; }" << "" << true
        << "foo" << "FuncPointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Function pointer invocations
    QTest::newRow("funcPtr34") << "int   (  foo)();" << "void* p = foo();" << true
        << "foo" << "Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr35") << "int*  (  foo)();" << "void* p = foo();" << true
        << "foo" << "Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr36") << "int** (  foo)();" << "void* p = foo();" << true
        << "foo" << "Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr37") << "int   (* foo)();" << "void* p = foo();" << true
        << "foo" << "Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr38") << "int*  (* foo)();" << "void* p = foo();" << true
        << "foo" << "Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr39") << "int** (* foo)();" << "void* p = foo();" << true
        << "foo" << "Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr40") << "int   (**foo)();" << "void* p = foo();" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr41") << "int*  (**foo)();" << "void* p = foo();" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr42") << "int** (**foo)();" << "void* p = foo();" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Pointers of function pointer typedefs
    QTest::newRow("funcPtr43") << "typedef int   (  foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr44") << "typedef int*  (  foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr45") << "typedef int** (  foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr46") << "typedef int   (* foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr47") << "typedef int*  (* foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr48") << "typedef int** (* foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr49") << "typedef int   (**foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr50") << "typedef int*  (**foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr51") << "typedef int** (**foodef)(); foodef *foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    QTest::newRow("funcPtr52") << "typedef int   (  foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr53") << "typedef int*  (  foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr54") << "typedef int** (  foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr55") << "typedef int   (* foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->Pointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr56") << "typedef int*  (* foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->Pointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr57") << "typedef int** (* foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->Pointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr58") << "typedef int   (**foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->Pointer->FuncPointer->FuncPointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr59") << "typedef int*  (**foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->Pointer->FuncPointer->FuncPointer->Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr60") << "typedef int** (**foodef)(); foodef **foo;" << "void* p = foo;" << true
        << "foo" << "Pointer->Pointer->FuncPointer->FuncPointer->Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Invocation of pointers of function pointer typedefs
    QTest::newRow("funcPtr61") << "typedef int   (  foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "foo" << "Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr62") << "typedef int*  (  foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "foo" << "Pointer->Int32" << "" << "Pointer->Void";
    QTest::newRow("funcPtr63") << "typedef int** (  foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "foo" << "Pointer->Pointer->Int32" << "" << "Pointer->Void";

    // Illegal invocation of pointers of function pointer typedefs
    QTest::newRow("funcPtr64") << "typedef int   (* foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "" << "" << "" << ""
        << "Expected a function pointer type here instead of \"Pointer->FuncPointer->Int32\" at :25:17";
    QTest::newRow("funcPtr65") << "typedef int*  (* foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "" << "" << "" << ""
        << "Expected a function pointer type here instead of \"Pointer->FuncPointer->Pointer->Int32\" at :25:17";
    QTest::newRow("funcPtr66") << "typedef int** (* foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "" << "" << "" << ""
        << "Expected a function pointer type here instead of \"Pointer->FuncPointer->Pointer->Pointer->Int32\" at :25:17";
    QTest::newRow("funcPtr67") << "typedef int   (**foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "" << "" << "" << ""
        << "Expected a function pointer type here instead of \"Pointer->FuncPointer->FuncPointer->Int32\" at :25:17";
    QTest::newRow("funcPtr68") << "typedef int*  (**foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "" << "" << "" << ""
        << "Expected a function pointer type here instead of \"Pointer->FuncPointer->FuncPointer->Pointer->Int32\" at :25:17";
    QTest::newRow("funcPtr69") << "typedef int** (**foodef)(); foodef *foo;" << "void* p = foo();" << true
        << "" << "" << "" << ""
        << "Expected a function pointer type here instead of \"Pointer->FuncPointer->FuncPointer->Pointer->Pointer->Int32\" at :25:17";
}


void ASTTypeEvaluatorTest::allTests()
{
	QFETCH(QString, globalCode);
	QFETCH(QString, localCode);

	_ascii += DEF_HEADER;
	if (!globalCode.isEmpty()) {
	    _ascii += globalCode.toAscii();
	    _ascii += "\n\n";
	}
	_ascii += DEF_MAIN_BEGIN;
	if (!localCode.isEmpty()) {
        _ascii += "    ";
	    _ascii += localCode.toAscii();
	    _ascii += "\n";
	}
	_ascii += DEF_MAIN_END;

	try {
        QCOMPARE(_builder->buildFrom(_ascii), 0);
        QCOMPARE(_tester->evaluateTypes(), true);

        QTEST(_tester->typeChanged, "typeChanged");

        if (_tester->typeChanged) {
            QTEST(_tester->symbolName, "symbolName");
            QTEST(_tester->ctxType, "ctxType");
            QTEST(_tester->ctxMembers, "ctxMembers");
            QTEST(_tester->targetType, "targetType");
        }
	}
	catch (GenericException& e) {
	    QFETCH(QString, exceptionMsg);
	    // Re-throw unexpected exceptions
	    if (exceptionMsg.isEmpty()) {
	        std::cerr << "Caught exception at " << qPrintable(e.file) << ":"
	                << e.line << ": " << qPrintable(e.message) << std::endl;
	        throw;
	    }
	    else {
	        QCOMPARE(e.message, exceptionMsg);
	    }
	}
	catch (...) {
	    cleanup();
	    throw;
	}

	_ascii.clear();
}

QTEST_MAIN(ASTTypeEvaluatorTest)