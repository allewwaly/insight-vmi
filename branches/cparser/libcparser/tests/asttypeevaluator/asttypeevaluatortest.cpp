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
		interLinks = 0;

		fSymbolName.clear();
		fCtxType.clear();
		fCtxMembers.clear();
		fTargetType.clear();

		lSymbolName.clear();
		lCtxType.clear();
		lCtxMembers.clear();
		lTargetType.clear();
	}

	bool typeChanged;
	QString fSymbolName;
	QString fCtxType;
	QString fCtxMembers;
	QString fTargetType;
	QString lSymbolName;
	QString lCtxType;
	QString lCtxMembers;
	QString lTargetType;
	int interLinks;

protected:
    virtual void primaryExpressionTypeChange(const TypeEvalDetails &ed)
    {
        // First type change, or the one with more interLinks
        if (!typeChanged || interLinks < ed.interLinks.size()) {
            typeChanged = true;
            fSymbolName = ed.sym->name();
            fCtxType = ed.ctxType->toString();
            fCtxMembers = ed.ctxMembers.join(".");
            fTargetType = ed.targetType->toString();
            interLinks = ed.interLinks.size();
        }
        // Last type change
        lSymbolName = ed.sym->name();
        lCtxType = ed.ctxType->toString();
        lCtxMembers = ed.ctxMembers.join(".");
        lTargetType = ed.targetType->toString();
    }

    int evaluateIntExpression(const ASTNode* /*node*/, bool* ok)
    {
        // Always return ok and zero, the value does not matter for these tests.
        if (ok)
            *ok = true;
        return 0;
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

#define STRING2(s) #s
#define STRING(s) STRING2(s)
#define LN "line: " STRING(__LINE__)


#define TEST_DATA_COLUMNS \
   QTest::addColumn<QString>("globalCode"); \
   QTest::addColumn<QString>("localCode"); \
   QTest::addColumn<int>("typeChanged"); \
   QTest::addColumn<QString>("symbolName"); \
   QTest::addColumn<QString>("ctxType"); \
   QTest::addColumn<QString>("ctxMembers"); \
   QTest::addColumn<QString>("targetType"); \
   QTest::addColumn<QString>("exceptionMsg");

#define NO_CHANGE(local_src) \
    NO_CHANGE2("", (local_src))
#define NO_CHANGE2(global_src, local_src) \
    TEST_CASE((global_src), (local_src), 0, "", "", "", "", "")

#define CHANGE_FIRST(local_src, sym_name, ctx_type, ctx_mem, target_type) \
    CHANGE_FIRST2("", (local_src), (sym_name), (ctx_type), (ctx_mem), (target_type))
#define CHANGE_FIRST2(global_src, local_src, sym_name, ctx_type, ctx_mem, target_type) \
    TEST_CASE((global_src), (local_src), 1, (sym_name), (ctx_type), (ctx_mem), (target_type), "")

#define CHANGE_LAST(local_src, sym_name, ctx_type, ctx_mem, target_type) \
    CHANGE_LAST2("", (local_src), (sym_name), (ctx_type), (ctx_mem), (target_type))
#define CHANGE_LAST2(global_src, local_src, sym_name, ctx_type, ctx_mem, target_type) \
    TEST_CASE((global_src), (local_src), -1, (sym_name), (ctx_type), (ctx_mem), (target_type), "")

#define EXCEPTIONAL(global_src, local_src, except_msg) \
    TEST_CASE((global_src), (local_src), 0, "", "", "", "", (except_msg))

#define TEST_CASE(global_src, local_src, which_changed, sym_name, ctx_type, ctx_mem, target_type, except_msg) \
    do { \
        QTest::newRow(LN) \
            << (global_src) \
            << (local_src) \
            << (which_changed) \
            << (sym_name) \
            << (ctx_type) \
            << (ctx_mem) \
            << (target_type) \
            << (except_msg); \
    } while (0)


#define	TEST_FUNCTION(methodName) \
	void ASTTypeEvaluatorTest::test_##methodName##_func() \
	{ \
		QFETCH(QString, globalCode); \
		QFETCH(QString, localCode); \
		QFETCH(int, typeChanged); \
	\
		_ascii += DEF_HEADER; \
		if (!globalCode.isEmpty()){ \
			_ascii += globalCode.toAscii(); \
			_ascii += "\n\n"; \
		} \
		_ascii += DEF_MAIN_BEGIN; \
		if (!localCode.isEmpty()){ \
			_ascii += "    "; \
			_ascii += localCode.toAscii(); \
			_ascii += "\n"; \
		} \
		_ascii += DEF_MAIN_END; \
	\
		try{ \
			QCOMPARE(_builder->buildFrom(_ascii), 0); \
			QCOMPARE(_tester->evaluateTypes(), true); \
	\
			if (!typeChanged) \
				QCOMPARE(_tester->typeChanged, false); \
			else  {\
				QCOMPARE(_tester->typeChanged, true); \
				if (typeChanged > 0) { \
					QTEST(_tester->fSymbolName, "symbolName"); \
					QTEST(_tester->fCtxType, "ctxType"); \
					QTEST(_tester->fCtxMembers, "ctxMembers"); \
					QTEST(_tester->fTargetType, "targetType"); \
				} \
				else if (typeChanged < 0) { \
					QTEST(_tester->lSymbolName, "symbolName"); \
					QTEST(_tester->lCtxType, "ctxType"); \
					QTEST(_tester->lCtxMembers, "ctxMembers"); \
					QTEST(_tester->lTargetType, "targetType"); \
				} \
			} \
		} \
		catch (GenericException& e) { \
			QFETCH(QString, exceptionMsg); \
			/* Re-throw unexpected exceptions */ \
			if (exceptionMsg.isEmpty()){ \
				std::cerr << "Caught exception at " << qPrintable(e.file) << ":" \
						<< e.line << ": " << qPrintable(e.message) << std::endl; \
				throw; \
			} \
			else{ \
				QCOMPARE(e.message, exceptionMsg); \
			} \
		} \
		catch (...){ \
			cleanup(); \
			throw; \
		} \
	\
		_ascii.clear(); \
	} \
	\
   void ASTTypeEvaluatorTest::test_##methodName##_func_data()


TEST_FUNCTION(basic)
{
	TEST_DATA_COLUMNS;

	// Very basic type equalities and changes
	NO_CHANGE("void* q; void *p = q;");
	NO_CHANGE("int i; int j = i;");
	NO_CHANGE("int i; char j = i;");
	NO_CHANGE("int i; char* p; p = p + i;");
	NO_CHANGE("int i; char* p; p = p - i;");
	NO_CHANGE("void* q, *p; p = q;");
	NO_CHANGE("int i, j; i = j;");
	NO_CHANGE("int i; char j; i = j;");
	NO_CHANGE("int i; char j; j = i;");
	NO_CHANGE("int i; char j; i += j;");
	NO_CHANGE("int i; char j; i -= j;");
	NO_CHANGE("int i; char j; i *= j;");
	NO_CHANGE("int i; char j; i /= j;");
	NO_CHANGE("int i; char j; i %= j;");
	NO_CHANGE("int i; char j; i <<= j;");
	NO_CHANGE("int i; char j; i >>= j;");
	NO_CHANGE("void* q = (1+2)*3;");
	CHANGE_LAST("int i; void *p = i;",
		"i", "Int32", "", "Pointer->Void");
	CHANGE_LAST("void *p; int i = p;",
		"p", "Pointer->Void", "", "Int32");
	CHANGE_LAST("int *i; char *j = i;",
		"i", "Pointer->Int32", "", "Pointer->Int8");
	CHANGE_LAST("int i; void *p; p = i;",
		"i", "Int32", "", "Pointer->Void");
	CHANGE_LAST("void *p; int i; i = p;",
		"p", "Pointer->Void", "", "Int32");
	CHANGE_LAST("int *i; char *j; i = j;",
		"j", "Pointer->Int8", "", "Pointer->Int32");
	CHANGE_LAST("int *i; char *j; j = i;",
		"i", "Pointer->Int32", "", "Pointer->Int8");
	NO_CHANGE("int i; char* p; p += i;");
	NO_CHANGE("int i; char* p; p -= i;");
	EXCEPTIONAL("", "int *i; char* p; p += i;", "Cannot determine resulting type of \"Pointer += Pointer\" at lines 23 and 23");
	EXCEPTIONAL("", "int *i; char* p; p -= i;", "Cannot determine resulting type of \"Pointer -= Pointer\" at lines 23 and 23");
	EXCEPTIONAL("", "int *i; char* p; p *= i;", "Cannot determine resulting type of \"Pointer *= Pointer\" at lines 23 and 23");
	EXCEPTIONAL("", "int *i; char* p; p >>= i;", "Cannot determine resulting type of \"Pointer >>= Pointer\" at lines 23 and 23");
}


TEST_FUNCTION(listHeadEqual)
{
	TEST_DATA_COLUMNS;

	// Various equality checks for list_head
	NO_CHANGE("h = heads[0];");
	NO_CHANGE("heads[0] = h;");
	NO_CHANGE("h = *heads;");
	NO_CHANGE("*heads = h;");
	NO_CHANGE("heads = &h;");
	NO_CHANGE("m->list.next = h;");
	NO_CHANGE("h = m->list.next;");
}


TEST_FUNCTION(postfixOperators)
{
	TEST_DATA_COLUMNS;

    // Array, arrow and star operator, all in the mix
    NO_CHANGE("h = m->plist->next;");
    NO_CHANGE("h = m[0].plist->next;");
    NO_CHANGE("h = m->plist[0].next;");
    NO_CHANGE("h = m[0].plist[0].next;");
    NO_CHANGE("h = (*m).plist->next;");
    NO_CHANGE("h = (*m->plist).next;");
    NO_CHANGE("h = (*m).plist[0].next;");
    NO_CHANGE("h = (*m[0].plist).next;");

    NO_CHANGE("*h = *m->plist->next;");
    NO_CHANGE("*h = *m[0].plist->next;");
    NO_CHANGE("*h = *m->plist[0].next;");
    NO_CHANGE("*h = *m[0].plist[0].next;");
    NO_CHANGE("*h = *(*m).plist->next;");
    NO_CHANGE("*h = *(*m->plist).next;");
    NO_CHANGE("*h = *(*m).plist[0].next;");
    NO_CHANGE("*h = *(*m[0].plist).next;");

    NO_CHANGE("*h = m->plist->next[0];");
    NO_CHANGE("*h = m[0].plist->next[0];");
    NO_CHANGE("*h = m->plist[0].next[0];");
    NO_CHANGE("*h = m[0].plist[0].next[0];");
    NO_CHANGE("*h = (*m).plist->next[0];");
    NO_CHANGE("*h = (*m->plist).next[0];");
    NO_CHANGE("*h = (*m).plist[0].next[0];");
    NO_CHANGE("*h = (*m[0].plist).next[0];");
}


TEST_FUNCTION(basicTypeCasts)
{
    TEST_DATA_COLUMNS;

    // Type casts that lead to equal types
    NO_CHANGE("h = (struct list_head*)m->list.next;");
    NO_CHANGE("h = (struct list_head*)((struct module*)m)->list.next;");
    NO_CHANGE("h = (struct list_head*)(*((struct module*)m)).list.next;");
}


TEST_FUNCTION(functionCalls)
{
    TEST_DATA_COLUMNS;

    // Function calls
    NO_CHANGE("h = func();");
    NO_CHANGE("*h = *func();");
    NO_CHANGE("h = func_ptr();");
    NO_CHANGE("*h = *func_ptr();");
}


TEST_FUNCTION(basicTypeChanges)
{
    TEST_DATA_COLUMNS;

    // Basic type changes
    CHANGE_LAST("h = m;",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("m = h;",
        "h", "Pointer->Struct(list_head)", "", "Pointer->Struct(module)");
    CHANGE_LAST("m = h->next;",
        "h", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("m = h->next->prev;",
        "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
}


TEST_FUNCTION(basicCastingChanges)
{
    TEST_DATA_COLUMNS;

    // Casting changes
    CHANGE_LAST("m = (struct module*)h->next;",
        "h", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("m = (struct list_head*)h->next;",
        "h", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("m = (void*)h->next;",
        "h", "Struct(list_head)", "next", "Pointer->Struct(module)");

    // list_head-like casting changes
    CHANGE_LAST("m = (struct module*)(((char*)modules.next) - __builtin_offsetof(struct module, list));",
        "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
}


TEST_FUNCTION(ignoredTypeChanges)
{
	TEST_DATA_COLUMNS;

    // Ignore type changes in certain cases
    NO_CHANGE("int i = sizeof((void*)m);");
    NO_CHANGE("(void*)m;");
    NO_CHANGE("h = (void*)(1+2);");
}



TEST_FUNCTION(structInitializers)
{
    TEST_DATA_COLUMNS;
    // Struct initializers
    NO_CHANGE("struct list_head foo = { h->next, h->prev };");
    NO_CHANGE("struct list_head foo = { m->list.next, m->list.prev };");
    NO_CHANGE("struct list_head foo = { .prev = h, .next = h };");
    NO_CHANGE("struct list_head foo = { &m->list, &m->list };");
    NO_CHANGE("struct module foo = { .foo = 0, .list = { h, h } };");
    NO_CHANGE("struct module foo = { .foo = 0 };");
    NO_CHANGE("struct module foo = { .list = { h, h } };");
    NO_CHANGE("struct module foo = { 0, { h, h } };");

    CHANGE_LAST("struct list_head foo = { .prev = m, .next = h };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct list_head foo = { .prev = (struct list_head*)m, .next = h };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct list_head foo = { .prev = (struct list_head*)m->foo, .next = h };",
        "m", "Struct(module)", "foo", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct module foo = { m, { h, h } };",
        "m", "Pointer->Struct(module)", "", "Int32");
    CHANGE_LAST("struct module foo = { 0, { m, h } };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct module foo = { .foo = m, .list = { h, h } };",
        "m", "Pointer->Struct(module)", "", "Int32");
    CHANGE_LAST("struct module foo = { .foo = 0, .list = { m, h } };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct module foo = { .foo = m };",
        "m", "Pointer->Struct(module)", "", "Int32");
    CHANGE_LAST("struct module foo = { .list = { m, h } };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct module foo = { .list = { h, m } };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST2("struct bar { int i; int (*func)(); };\nint ifunc() { return 0; }", "struct bar foo = { .func = m };",
        "m", "Pointer->Struct(module)", "", "FuncPointer->Int32");
    NO_CHANGE2("struct bar { int i; int (*func)(); };\nint ifunc() { return 0; }", "struct bar foo = { .func = ifunc };");
}


TEST_FUNCTION(designatedInitializers)
{
	TEST_DATA_COLUMNS;
	// Designated struct initializers
	NO_CHANGE2("struct list_head foo;\n", "foo = (struct list_head){ h->next, h->prev };");
	NO_CHANGE2("struct list_head foo;\n", "foo = (struct list_head){ m->list.next, m->list.prev };");
	NO_CHANGE2("struct list_head foo;\n", "foo = (struct list_head){ .prev = h, .next = h };");
	NO_CHANGE2("struct list_head foo;\n", "foo = (struct list_head){ &m->list, &m->list };");
	NO_CHANGE2("struct module foo;\n", "foo = (struct module){ .foo = 0, .list = { h, h } };");
	NO_CHANGE2("struct module foo;\n", "foo = (struct module){ 0, { h, h } };");

    CHANGE_LAST2("struct list_head foo;\n", "struct list_head foo = { .prev = m, .next = h };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST2("struct list_head foo;\n", "struct list_head foo = { .prev = (struct list_head*)m, .next = h };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST2("struct list_head foo;\n", "struct list_head foo = { .prev = (struct list_head*)m->foo, .next = h };",
        "m", "Struct(module)", "foo", "Pointer->Struct(list_head)");
    CHANGE_LAST2("struct module foo;\n", "foo = (struct module){ m, { h, h } };",
        "m", "Pointer->Struct(module)", "", "Int32");
    CHANGE_LAST2("struct module foo;\n", "foo = (struct module){ 0, { m, h } };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST2("struct module foo;\n", "foo = (struct module){ h, { 0, 0 } };",
        "h", "Pointer->Struct(list_head)", "", "Int32");
    CHANGE_LAST2("struct module foo;\n", "foo = (struct module){ .foo = m, .list = { h, h } };",
        "m", "Pointer->Struct(module)", "", "Int32");
    CHANGE_LAST2("struct module foo;\n", "foo = (struct module){ .foo = 0, .list = { m, h } };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST2("struct module foo;\n", "foo = (struct module){ .foo = h, .list = { 0, 0 } };",
        "h", "Pointer->Struct(list_head)", "", "Int32");
}


TEST_FUNCTION(arrayInitializers)
{
    TEST_DATA_COLUMNS;
    // Array initializers
    NO_CHANGE("struct list_head a[2] = { {h, h}, {h, h} };");
    NO_CHANGE("struct list_head a[2] = { [0] = {h, h} };");
    NO_CHANGE("struct list_head a[2] = { [0] = { .prev = h, .next = h } };");
    NO_CHANGE("struct module* a[2] = { m, m };");
    NO_CHANGE("struct module* a[2] = { [0] = m };");
    NO_CHANGE2("enum en { enVal1, enVal2, enSize };",
              "int array[enSize][enSize] = { [enVal1] = { 0, 1 }, [enVal2] = { 2, 3 } };");

    CHANGE_LAST("struct list_head a[2] = { {h, m}, {h, h} };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct list_head a[2] = { {h, h}, {h, m} };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct list_head a[2] = { [0] = {h, m} };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct list_head a[2] = { [0] = { .prev = m, .next = h} };",
        "m", "Pointer->Struct(module)", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("struct module* a[2] = { h->next, m };",
        "h", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("struct module* a[2] = { h->prev, m };",
        "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
    CHANGE_LAST("struct module* a[2] = { [0] = h->prev };",
        "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");

}


TEST_FUNCTION(returnValues)
{
    TEST_DATA_COLUMNS;
    // Return values
    NO_CHANGE2("struct module* foo() { return; }", "");
    NO_CHANGE2("struct module* foo() { return 0; }", "");
    NO_CHANGE2("struct module* foo(struct module* p) { return p; }", "");
    CHANGE_LAST2("struct module* foo(void* p) { return p; }", "",
        "p", "Pointer->Void", "", "Pointer->Struct(module)");
    CHANGE_LAST2("struct module* foo(long i) { return i; }", "",
        "i", "Int32", "", "Pointer->Struct(module)");
}


TEST_FUNCTION(functionDefsDecls)
{
    TEST_DATA_COLUMNS;
    // Function definitions
    CHANGE_LAST2("int foo() { return 0; }", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int* foo() { return 0; }", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** foo() { return 0; }", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");

    // Function declarations
    CHANGE_LAST2("int foo();", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int* foo();", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** foo();", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
}


TEST_FUNCTION(functionPointers)
{
    TEST_DATA_COLUMNS;
    // Function pointers
    CHANGE_LAST2("int   (  foo)();", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int*  (  foo)();", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** (  foo)();", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int   (* foo)();", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int*  (* foo)();", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** (* foo)();", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int   (**foo)();", "void* p = foo;",
        "foo", "FuncPointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int*  (**foo)();", "void* p = foo;",
        "foo", "FuncPointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** (**foo)();", "void* p = foo;",
        "foo", "FuncPointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
}


TEST_FUNCTION(functionPointerTypedefs)
{
    TEST_DATA_COLUMNS;
    // Function pointer typedefs
    CHANGE_LAST2("typedef int   (  foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (  foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (  foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int   (* foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (* foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (* foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int   (**foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (**foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (**foodef)(); foodef foo;", "void* p = foo;",
        "foo", "FuncPointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
}


TEST_FUNCTION(functionPointerParams)
{
    TEST_DATA_COLUMNS;
    // Function pointer parameters
    CHANGE_LAST2("void func(int   (  foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int*  (  foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int** (  foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int   (* foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int*  (* foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int** (* foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int   (**foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int*  (**foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("void func(int** (**foo)()) { void* p = foo; }", "",
        "foo", "FuncPointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
}


TEST_FUNCTION(functionPointerInvocations)
{
    TEST_DATA_COLUMNS;
    // Function pointer invocations
    CHANGE_LAST2("int   (  foo)();", "void* p = foo();",
        "foo", "Int32", "", "Pointer->Void");
    CHANGE_LAST2("int*  (  foo)();", "void* p = foo();",
        "foo", "Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** (  foo)();", "void* p = foo();",
        "foo", "Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int   (* foo)();", "void* p = foo();",
        "foo", "Int32", "", "Pointer->Void");
    CHANGE_LAST2("int*  (* foo)();", "void* p = foo();",
        "foo", "Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** (* foo)();", "void* p = foo();",
        "foo", "Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int   (**foo)();", "void* p = foo();",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int*  (**foo)();", "void* p = foo();",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("int** (**foo)();", "void* p = foo();",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
}


TEST_FUNCTION(funcPtrTypdefPtrs)
{
    TEST_DATA_COLUMNS;
    // Pointers of function pointer typedefs
    CHANGE_LAST2("typedef int   (  foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (  foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (  foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int   (* foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (* foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (* foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int   (**foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (**foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (**foodef)(); foodef *foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");

    CHANGE_LAST2("typedef int   (  foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (  foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (  foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int   (* foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->Pointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (* foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->Pointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (* foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->Pointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int   (**foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->Pointer->FuncPointer->FuncPointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (**foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->Pointer->FuncPointer->FuncPointer->Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (**foodef)(); foodef **foo;", "void* p = foo;",
        "foo", "Pointer->Pointer->FuncPointer->FuncPointer->Pointer->Pointer->Int32", "", "Pointer->Void");
}


TEST_FUNCTION(funcPtrTypdefPtrInvocations)
{
    TEST_DATA_COLUMNS;
    // Invocation of pointers of function pointer typedefs
    CHANGE_LAST2("typedef int   (  foodef)(); foodef *foo;", "void* p = foo();",
        "foo", "Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int*  (  foodef)(); foodef *foo;", "void* p = foo();",
        "foo", "Pointer->Int32", "", "Pointer->Void");
    CHANGE_LAST2("typedef int** (  foodef)(); foodef *foo;", "void* p = foo();",
        "foo", "Pointer->Pointer->Int32", "", "Pointer->Void");

    // Illegal invocation of pointers of function pointer typedefs
    EXCEPTIONAL("typedef int   (* foodef)(); foodef *foo;", "void* p = foo();",
        "Expected a function pointer type here instead of \"Pointer->FuncPointer->Int32\" at :25:17");
    EXCEPTIONAL("typedef int*  (* foodef)(); foodef *foo;", "void* p = foo();",
        "Expected a function pointer type here instead of \"Pointer->FuncPointer->Pointer->Int32\" at :25:17");
    EXCEPTIONAL("typedef int** (* foodef)(); foodef *foo;", "void* p = foo();",
        "Expected a function pointer type here instead of \"Pointer->FuncPointer->Pointer->Pointer->Int32\" at :25:17");
    EXCEPTIONAL("typedef int   (**foodef)(); foodef *foo;", "void* p = foo();",
        "Expected a function pointer type here instead of \"Pointer->FuncPointer->FuncPointer->Int32\" at :25:17");
    EXCEPTIONAL("typedef int*  (**foodef)(); foodef *foo;", "void* p = foo();",
        "Expected a function pointer type here instead of \"Pointer->FuncPointer->FuncPointer->Pointer->Int32\" at :25:17");
    EXCEPTIONAL("typedef int** (**foodef)(); foodef *foo;", "void* p = foo();",
        "Expected a function pointer type here instead of \"Pointer->FuncPointer->FuncPointer->Pointer->Pointer->Int32\" at :25:17");
}


TEST_FUNCTION(logicalExpressions)
{
    TEST_DATA_COLUMNS;

    // Usage in logical expressions
    NO_CHANGE("char *p, *q; int i; i = p == q;");
    NO_CHANGE("char *p, *q; int i; i = p == i;");
    NO_CHANGE("char *p, *q; int i = p == q;");
    NO_CHANGE("char *p, *q; int i = p == i;");
    NO_CHANGE("char *p, *q; int i; i = p != q;");
    NO_CHANGE("char *p, *q; int i; i = p != i;");
    NO_CHANGE("char *p, *q; int i = p != q;");
    NO_CHANGE("char *p, *q; int i = p != i;");

    NO_CHANGE("char *p, *q; int i; i = p < q;");
    NO_CHANGE("char *p, *q; int i; i = p <= q;");
    NO_CHANGE("char *p, *q; int i; i = p > q;");
    NO_CHANGE("char *p, *q; int i; i = p >= q;");

    NO_CHANGE("char *p, *q; int i; i = p && q;");
    NO_CHANGE("char *p, *q; int i; i = p || q;");
    NO_CHANGE("char *p; int i; i = !p;");
}


TEST_FUNCTION(castExpressions)
{
    TEST_DATA_COLUMNS;
    // Cast expressions
    NO_CHANGE("char *p, *q; p = (char*)q;");
    CHANGE_LAST("char *p; int i; p = (char*)i;",
        "i", "Int32", "", "Pointer->Int8");
    NO_CHANGE("char *p ,*q; int i; p = (char*)(q + i);");
    NO_CHANGE("void *p; int i; p = (void*)((char*)p + i);");
    NO_CHANGE("char *p; int i; p = (void*)((char*)p + i);");
    CHANGE_LAST("void *p; char* q; int i; p = (void*)((char*)q + i);",
        "q", "Pointer->Int8", "", "Pointer->Void");
    CHANGE_LAST("void *p; char* q; int i; p = (void*)(q + i);",
        "q", "Pointer->Int8", "", "Pointer->Void");
    CHANGE_LAST("void *p; char* q; int i; q = (void*)((char*)p + i);",
        "p", "Pointer->Void", "", "Pointer->Int8");
    CHANGE_LAST("void *p; char* q; int i; q = ((char*)p + i);",
        "p", "Pointer->Void", "", "Pointer->Int8");
    NO_CHANGE("int *p; int i; i = *p;");
    CHANGE_LAST("void *p; int i; i = *((char*)p);",
        "p", "Pointer->Void", "", "Pointer->Int8");
    CHANGE_LAST("void *p; int i; i = ((char*)p);",
        "p", "Pointer->Void", "", "Int32");
    NO_CHANGE("modules.next = (h)->next;");
    NO_CHANGE("modules.next = ((struct list_head *)h)->next;");
    NO_CHANGE("modules.next = ((h))->next;");
    NO_CHANGE("modules.next = (((struct list_head *)h))->next;");
    NO_CHANGE("modules.next = ((struct list_head *)(h))->next;");
    NO_CHANGE("modules.next = (struct list_head *)(h)->next;");
    NO_CHANGE("modules.next = (struct list_head *)h->next;");
    CHANGE_LAST("void *p; modules.next = ((struct list_head *)p)->next;",
                "p", "Pointer->Void", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("void *p; modules.next = (((struct list_head *)p))->next;",
                "p", "Pointer->Void", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("void *p; modules.next = ((struct list_head *)(p))->next;",
                "p", "Pointer->Void", "", "Pointer->Struct(list_head)");

    CHANGE_LAST("void *p; m = (struct module*)((struct list_head*)p)->next;",
                "p", "Pointer->Void", "", "Pointer->Struct(list_head)");
    CHANGE_LAST("m = (struct module*)(h)->next;",
                "h", "Struct(list_head)", "next", "Pointer->Struct(module)");
}


TEST_FUNCTION(conditionalExpressions)
{
    TEST_DATA_COLUMNS;
    // Cast expressions
    NO_CHANGE2("struct bus_type { char* name; }; "
              "struct dev_type { struct bus_type* bus; }; "
              "char* f(struct dev_type* dev) { return dev->bus ? dev->bus->name : \"\"; }", "");
    CHANGE_LAST2("struct bus_type { char* name; }; "
                "struct dev_type { struct bus_type* bus; }; "
                "void* f(struct dev_type* dev) { return dev->bus ? dev->bus->name : \"\"; }",
                "",
                "dev", "Struct(bus_type)", "name", "Pointer->Void");
}


TEST_FUNCTION(pointerDerefByArrayOperator)
{
    TEST_DATA_COLUMNS;
    // For pointers dereferenced by an array operator, the context type is the
    // type embedding the pointer member, in this case "struct foo"
    CHANGE_LAST2("struct foo { struct foo* next; }; struct bar { struct foo *f; };",
                "struct bar b; void *p = b.f[0].next;",
                "b", "Struct(foo)", "next", "Pointer->Void");
    // For arrays dereferenced by an array operator, the context type is the
    // type embedding the array, in this case "struct bar"
    CHANGE_LAST2("struct foo { struct foo* next; }; struct bar { struct foo f[4]; };",
                "struct bar b; void *p = b.f[0].next;",
                "b", "Struct(bar)", "f.next", "Pointer->Void");
    // If the source symbol is dereferenced, there is no difference in whether
    // it was defined as an array or as a pointer.
    CHANGE_LAST2("struct foo { struct foo* next; };",
                "struct foo *f; void *p = f[0].next;",
                "f", "Struct(foo)", "next", "Pointer->Void");
    CHANGE_LAST2("struct foo { struct foo* next; };",
                "struct foo f[4]; void *p = f[0].next;",
                "f", "Struct(foo)", "next", "Pointer->Void");
}


TEST_FUNCTION(compoundBraces)
{
    TEST_DATA_COLUMNS;
    NO_CHANGE("modules.next = ({ int i = 0, j = 1; i += j; h->next; });");
    CHANGE_LAST("modules.next = ({ void *p; p; });",
                "p", "Pointer->Void", "", "Pointer->Struct(list_head)");

}


TEST_FUNCTION(transitiveEvaluation)
{
    TEST_DATA_COLUMNS;
    // Simple transitive change
    CHANGE_FIRST("void *p = modules.next; m = p;",
                "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("void *p = modules.next; m = p;",
                 "p", "Pointer->Void", "", "Pointer->Struct(module)");
    // Double indirect change through void
    CHANGE_FIRST("void *p = modules.next, *q = p; m = q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("void *p = modules.next, *q = p; m = q;",
                "q", "Pointer->Void", "", "Pointer->Struct(module)");
    // Indirect change through list_head
    CHANGE_FIRST("h = modules.next; m = h;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("h = modules.next; m = h;",
                "h", "Pointer->Struct(list_head)", "", "Pointer->Struct(module)");
    // Doubly indirect change through list_head and void
    CHANGE_FIRST("h = modules.next; void* p = h; m = p;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("h = modules.next; void* p = h; m = p;",
                "p", "Pointer->Void", "", "Pointer->Struct(module)");
    // Indirect change through long
    CHANGE_FIRST("long i = (long)modules.next; m = (struct modules*)i;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST("long i = (long)modules.next; m = (struct modules*)i;",
                "i", "Int32", "", "Pointer->Struct(module)");
    // Order of statements does not matter
    CHANGE_FIRST("m = h; h = modules.next;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through function
    CHANGE_FIRST2("void* getModule() { return modules.next; }",
                  "m = getModule();",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through two functions
    CHANGE_FIRST2("void* getModule() { return modules.next; }"
                  "void* getModule2() { return getModule(); }",
                  "m = getModule2();",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through function and variable
    CHANGE_FIRST2("void* getModule() { return modules.next; }",
                  "long l = getModule(); m = l;",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through two functions and variable
    CHANGE_FIRST2("void* getModule() { return modules.next; }"
                  "void* getModule2() { return getModule(); }",
                  "long l = getModule2(); m = l;",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through function and variable
    CHANGE_FIRST2("struct list_head* getModule() { return modules.next; }",
                  "h = getModule(); m = h;",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_LAST2("struct list_head* getModule() { return modules.next; }",
                 "h = getModule(); m = h;",
                 "h", "Pointer->Struct(list_head)", "", "Pointer->Struct(module)");
    NO_CHANGE2("struct list_head* getModule() { return modules.next; }",
              "h = getModule();");
    // Transitivity through 1 indirect pointer
    CHANGE_FIRST("void *p, **q; q = &p; p = modules.next; m = p;",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q; q = &p; p = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q; q = &p; *q = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q; q = &p; *q = modules.next; m = p;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");

    CHANGE_FIRST("void *p = modules.next, **q = &p; m = p;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p = modules.next, **q = &p; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q = &p; *q  = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q = &p; *q  = modules.next; m = p;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through 2 indirect pointers
    CHANGE_FIRST("void *p, **q, ***r; q = &p; r = &q; p = modules.next; m = *q;",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q, ***r; q = &p; r = &q; p = modules.next; m = **r;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q, ***r; q = &p; r = &q; *q = modules.next; m = p;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q, ***r; q = &p; r = &q; *q = modules.next; m = **r;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q, ***r; q = &p; r = &q; **r = modules.next; m = p;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("void *p, **q, ***r; q = &p; r = &q; **r = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    // Transitivity through 1 direct pointer
    CHANGE_FIRST("void *p, **q; p = modules.next; *q = p; m = *q;",
                  "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");

    // Pointer sensitivity
    CHANGE_FIRST("struct list_head **q; *q = modules.next; m = q;",
                 "q", "Pointer->Pointer->Struct(list_head)", "", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head **q; *q = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head **q; q = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head *p, **q; q = &p; p = modules.next; m = q;",
                 "q", "Pointer->Pointer->Struct(list_head)", "", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head *p, **q; q = &p; p = modules.next; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head *p, **q; p = modules.next; *q = p; m = *q;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");

    // Field sensitivity of assignments
    CHANGE_FIRST("h->prev = modules.next; m = h->prev;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("h->prev->prev = modules.next; m = h->prev->prev;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("h->prev->next = modules.next; m = h->prev->next;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("h->prev = modules.next; m = h->prev->prev;",
                 "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
    CHANGE_FIRST("h->prev->prev = modules.next; m = h->prev;",
                 "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
    CHANGE_FIRST("h->next = modules.next; m = h->prev;",
                 "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
    CHANGE_FIRST("h = modules.next; m = h->prev;",
                 "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
    CHANGE_FIRST("h->next = modules.next; m = h;",
                 "h", "Pointer->Struct(list_head)", "", "Pointer->Struct(module)");

    // Combination of bracket and arrow expressions
    CHANGE_FIRST("struct list_head **ph; ph[0]->prev = modules.next; m = ph[0]->prev;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head **ph; ph[0]->prev = modules.next; m = ph[1]->prev;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head **ph; ph[0]->prev = modules.next; m = ph[0]->next;",
                 "ph", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("struct list_head **ph; ph[0]->prev = modules.next; m = ph[0];",
                 "ph", "Pointer->Struct(list_head)", "", "Pointer->Struct(module)");

    // Nested expressions that work
    CHANGE_FIRST("(h)->prev = modules.next; m = h->prev;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");
    CHANGE_FIRST("h->prev = modules.next; m = (h)->prev;",
                 "modules", "Struct(list_head)", "next", "Pointer->Struct(module)");

    // Nested expressions that are two different pointers to us, even though
    // they are semantically the same
    CHANGE_FIRST("(*h).prev = modules.next; m = h->prev;",
                 "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");
    CHANGE_FIRST("h->prev = modules.next; m = (*h).prev;",
                 "h", "Struct(list_head)", "prev", "Pointer->Struct(module)");

}


QTEST_MAIN(ASTTypeEvaluatorTest)
