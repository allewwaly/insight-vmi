#include "astexpressionevaluatortester.h"

#include <QBuffer>
#include <abstractsyntaxtree.h>
#include <astbuilder.h>
#include <asttypeevaluator.h>
#include "../../memspecs.h"
#include "../../symfactory.h"
#include "../../astexpressionevaluator.h"
#include "../../kernelsymbolparser.h"
#include "../../kernelsourcetypeevaluator.h"
#include "../../shell.h"

#define safe_delete(x) \
    do { if ((x)) { delete (x); (x) = 0; } } while (0)

const char* test_prog_begin =
        "struct A {\n"
        "       long l;\n"
        "       short s;\n"
        "       int i;\n"
        "       char c;\n"
        "};\n"
        "\n"
        "struct B {\n"
        "       int j;\n"
        "       struct A a[4];\n"
        "       struct A* pa;\n"
        "       struct B* pb;\n"
        "};\n"
        "\n"
        "struct A a;\n"
        "struct B b;\n"
        "\n"
        "int main(int argc, char** argv)\n"
        "{\n";

const char* test_prog_end =
        "\n"
        "}\n"
        "\n";

// Created with the following command:
// gcc -g -o test test.c && objdump -W test | grep '^\s*<' | sed 's/^.*$/"\0\\n"/'
const char* objdump =
        " <0><b>: Abbrev Number: 1 (DW_TAG_compile_unit)\n"
        "    < c>   DW_AT_producer    : (indirect string, offset: 0x0): GNU C 4.5.2     \n"
        "    <10>   DW_AT_language    : 1       (ANSI C)\n"
        "    <11>   DW_AT_name        : (indirect string, offset: 0x28): test.c \n"
        "    <15>   DW_AT_comp_dir    : (indirect string, offset: 0x4c): insight-vmi/insightd/tests/astexpressionevaluator     \n"
        "    <19>   DW_AT_low_pc      : 0x4004b4        \n"
        "    <21>   DW_AT_high_pc     : 0x4004e4        \n"
        "    <29>   DW_AT_stmt_list   : 0x0     \n"
        " <1><2d>: Abbrev Number: 2 (DW_TAG_base_type)\n"
        "    <2e>   DW_AT_byte_size   : 8       \n"
        "    <2f>   DW_AT_encoding    : 5       (signed)\n"
        "    <30>   DW_AT_name        : (indirect string, offset: 0x2f): long int       \n"
        " <1><34>: Abbrev Number: 2 (DW_TAG_base_type)\n"
        "    <35>   DW_AT_byte_size   : 8       \n"
        "    <36>   DW_AT_encoding    : 7       (unsigned)\n"
        "    <37>   DW_AT_name        : (indirect string, offset: 0xc): long unsigned int       \n"
        " <1><3b>: Abbrev Number: 3 (DW_TAG_base_type)\n"
        "    <3c>   DW_AT_byte_size   : 4       \n"
        "    <3d>   DW_AT_encoding    : 5       (signed)\n"
        "    <3e>   DW_AT_name        : int     \n"
        " <1><42>: Abbrev Number: 4 (DW_TAG_structure_type)\n"
        "    <43>   DW_AT_name        : A       \n"
        "    <45>   DW_AT_byte_size   : 24      \n"
        "    <46>   DW_AT_decl_file   : 1       \n"
        "    <47>   DW_AT_decl_line   : 4       \n"
        "    <48>   DW_AT_sibling     : <0x7d>  \n"
        " <2><4c>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <4d>   DW_AT_name        : l       \n"
        "    <4f>   DW_AT_decl_file   : 1       \n"
        "    <50>   DW_AT_decl_line   : 5       \n"
        "    <51>   DW_AT_type        : <0x2d>  \n"
        "    <55>   DW_AT_data_member_location: 2 byte block: 23 0      (DW_OP_plus_uconst: 0)\n"
        " <2><58>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <59>   DW_AT_name        : s       \n"
        "    <5b>   DW_AT_decl_file   : 1       \n"
        "    <5c>   DW_AT_decl_line   : 6       \n"
        "    <5d>   DW_AT_type        : <0x7d>  \n"
        "    <61>   DW_AT_data_member_location: 2 byte block: 23 8      (DW_OP_plus_uconst: 8)\n"
        " <2><64>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <65>   DW_AT_name        : i       \n"
        "    <67>   DW_AT_decl_file   : 1       \n"
        "    <68>   DW_AT_decl_line   : 7       \n"
        "    <69>   DW_AT_type        : <0x3b>  \n"
        "    <6d>   DW_AT_data_member_location: 2 byte block: 23 c      (DW_OP_plus_uconst: 12)\n"
        " <2><70>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <71>   DW_AT_name        : c       \n"
        "    <73>   DW_AT_decl_file   : 1       \n"
        "    <74>   DW_AT_decl_line   : 8       \n"
        "    <75>   DW_AT_type        : <0x84>  \n"
        "    <79>   DW_AT_data_member_location: 2 byte block: 23 10     (DW_OP_plus_uconst: 16)\n"
        " <1><7d>: Abbrev Number: 2 (DW_TAG_base_type)\n"
        "    <7e>   DW_AT_byte_size   : 2       \n"
        "    <7f>   DW_AT_encoding    : 5       (signed)\n"
        "    <80>   DW_AT_name        : (indirect string, offset: 0x42): short int      \n"
        " <1><84>: Abbrev Number: 2 (DW_TAG_base_type)\n"
        "    <85>   DW_AT_byte_size   : 1       \n"
        "    <86>   DW_AT_encoding    : 6       (signed char)\n"
        "    <87>   DW_AT_name        : (indirect string, offset: 0x23): char   \n"
        " <1><8b>: Abbrev Number: 4 (DW_TAG_structure_type)\n"
        "    <8c>   DW_AT_name        : B       \n"
        "    <8e>   DW_AT_byte_size   : 120     \n"
        "    <8f>   DW_AT_decl_file   : 1       \n"
        "    <90>   DW_AT_decl_line   : 11      \n"
        "    <91>   DW_AT_sibling     : <0xc8>  \n"
        " <2><95>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <96>   DW_AT_name        : j       \n"
        "    <98>   DW_AT_decl_file   : 1       \n"
        "    <99>   DW_AT_decl_line   : 12      \n"
        "    <9a>   DW_AT_type        : <0x3b>  \n"
        "    <9e>   DW_AT_data_member_location: 2 byte block: 23 0      (DW_OP_plus_uconst: 0)\n"
        " <2><a1>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <a2>   DW_AT_name        : a       \n"
        "    <a4>   DW_AT_decl_file   : 1       \n"
        "    <a5>   DW_AT_decl_line   : 13      \n"
        "    <a6>   DW_AT_type        : <0xc8>  \n"
        "    <aa>   DW_AT_data_member_location: 2 byte block: 23 8      (DW_OP_plus_uconst: 8)\n"
        " <2><ad>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <ae>   DW_AT_name        : pa      \n"
        "    <b1>   DW_AT_decl_file   : 1       \n"
        "    <b2>   DW_AT_decl_line   : 14      \n"
        "    <b3>   DW_AT_type        : <0xd8>  \n"
        "    <b7>   DW_AT_data_member_location: 2 byte block: 23 68     (DW_OP_plus_uconst: 104)\n"
        " <2><ba>: Abbrev Number: 5 (DW_TAG_member)\n"
        "    <bb>   DW_AT_name        : pb      \n"
        "    <be>   DW_AT_decl_file   : 1       \n"
        "    <bf>   DW_AT_decl_line   : 15      \n"
        "    <c0>   DW_AT_type        : <0xde>  \n"
        "    <c4>   DW_AT_data_member_location: 2 byte block: 23 70     (DW_OP_plus_uconst: 112)\n"
        " <1><c8>: Abbrev Number: 6 (DW_TAG_array_type)\n"
        "    <c9>   DW_AT_type        : <0x42>  \n"
        "    <cd>   DW_AT_sibling     : <0xd8>  \n"
        " <2><d1>: Abbrev Number: 7 (DW_TAG_subrange_type)\n"
        "    <d2>   DW_AT_type        : <0x34>  \n"
        "    <d6>   DW_AT_upper_bound : 3       \n"
        " <1><d8>: Abbrev Number: 8 (DW_TAG_pointer_type)\n"
        "    <d9>   DW_AT_byte_size   : 8       \n"
        "    <da>   DW_AT_type        : <0x42>  \n"
        " <1><de>: Abbrev Number: 8 (DW_TAG_pointer_type)\n"
        "    <df>   DW_AT_byte_size   : 8       \n"
        "    <e0>   DW_AT_type        : <0x8b>  \n"
        " <1><e4>: Abbrev Number: 9 (DW_TAG_subprogram)\n"
        "    <e5>   DW_AT_external    : 1       \n"
        "    <e6>   DW_AT_name        : (indirect string, offset: 0x1e): main   \n"
        "    <ea>   DW_AT_decl_file   : 1       \n"
        "    <eb>   DW_AT_decl_line   : 21      \n"
        "    <ec>   DW_AT_prototyped  : 1       \n"
        "    <ed>   DW_AT_type        : <0x3b>  \n"
        "    <f1>   DW_AT_low_pc      : 0x4004b4        \n"
        "    <f9>   DW_AT_high_pc     : 0x4004e4        \n"
        "    <101>   DW_AT_frame_base  : 0x0    (location list)\n"
        "    <105>   DW_AT_sibling     : <0x126>        \n"
        " <2><109>: Abbrev Number: 10 (DW_TAG_formal_parameter)\n"
        "    <10a>   DW_AT_name        : (indirect string, offset: 0x38): argc  \n"
        "    <10e>   DW_AT_decl_file   : 1      \n"
        "    <10f>   DW_AT_decl_line   : 21     \n"
        "    <110>   DW_AT_type        : <0x3b> \n"
        "    <114>   DW_AT_location    : 2 byte block: 91 6c    (DW_OP_fbreg: -20)\n"
        " <2><117>: Abbrev Number: 10 (DW_TAG_formal_parameter)\n"
        "    <118>   DW_AT_name        : (indirect string, offset: 0x3d): argv  \n"
        "    <11c>   DW_AT_decl_file   : 1      \n"
        "    <11d>   DW_AT_decl_line   : 21     \n"
        "    <11e>   DW_AT_type        : <0x126>        \n"
        "    <122>   DW_AT_location    : 2 byte block: 91 60    (DW_OP_fbreg: -32)\n"
        " <1><126>: Abbrev Number: 8 (DW_TAG_pointer_type)\n"
        "    <127>   DW_AT_byte_size   : 8      \n"
        "    <128>   DW_AT_type        : <0x12c>        \n"
        " <1><12c>: Abbrev Number: 8 (DW_TAG_pointer_type)\n"
        "    <12d>   DW_AT_byte_size   : 8      \n"
        "    <12e>   DW_AT_type        : <0x84> \n"
        " <1><132>: Abbrev Number: 11 (DW_TAG_variable)\n"
        "    <133>   DW_AT_name        : a      \n"
        "    <135>   DW_AT_decl_file   : 1      \n"
        "    <136>   DW_AT_decl_line   : 18     \n"
        "    <137>   DW_AT_type        : <0x42> \n"
        "    <13b>   DW_AT_external    : 1      \n"
        "    <13c>   DW_AT_declaration : 1      \n"
        " <1><13d>: Abbrev Number: 11 (DW_TAG_variable)\n"
        "    <13e>   DW_AT_name        : b      \n"
        "    <140>   DW_AT_decl_file   : 1      \n"
        "    <141>   DW_AT_decl_line   : 19     \n"
        "    <142>   DW_AT_type        : <0x8b> \n"
        "    <146>   DW_AT_external    : 1      \n"
        "    <147>   DW_AT_declaration : 1      \n"
        " <1><148>: Abbrev Number: 12 (DW_TAG_variable)\n"
        "    <149>   DW_AT_name        : a      \n"
        "    <14b>   DW_AT_decl_file   : 1      \n"
        "    <14c>   DW_AT_decl_line   : 18     \n"
        "    <14d>   DW_AT_type        : <0x42> \n"
        "    <151>   DW_AT_external    : 1      \n"
        "    <152>   DW_AT_location    : 9 byte block: 3 40 10 60 0 0 0 0 0     (DW_OP_addr: 601040)\n"
        " <1><15c>: Abbrev Number: 12 (DW_TAG_variable)\n"
        "    <15d>   DW_AT_name        : b      \n"
        "    <15f>   DW_AT_decl_file   : 1      \n"
        "    <160>   DW_AT_decl_line   : 19     \n"
        "    <161>   DW_AT_type        : <0x8b> \n"
        "    <165>   DW_AT_external    : 1      \n"
        "    <166>   DW_AT_location    : 9 byte block: 3 60 10 60 0 0 0 0 0     (DW_OP_addr: 601060)\n"
        "\n";

QTEST_MAIN(ASTExpressionEvaluatorTester)


class ASTExpressionTester: public ASTWalker
{
public:
    ASTExpressionTester(AbstractSyntaxTree* ast, SymFactory* factory,
                        ASTTypeEvaluator* eval)
        : ASTWalker(ast), _expr(eval, factory) {}

    ExpressionResult result;

protected:
    void afterChildren(const ASTNode *node, int flags)
    {
        Q_UNUSED(flags);
        ASTExpression *e = 0;
        ASTNodeNodeHash ptsTo;

        if (node && node->type == nt_assignment_expression &&
            node->u.assignment_expression.assignment_expression)
            e = _expr.exprOfNode(
                        node->u.assignment_expression.assignment_expression,
                        ptsTo);
        else if (node && node->type == nt_initializer &&
                 node->u.initializer.assignment_expression)
            e = _expr.exprOfNode(
                        node->u.initializer.assignment_expression,
                        ptsTo);

        if (e)
            result = e->result();
    }

private:
    SymFactory* _factory;
    ASTExpressionEvaluator _expr;
};


ASTExpressionEvaluatorTester::ASTExpressionEvaluatorTester(QObject *parent) :
    QObject(parent), _specs(0), _factory(0), _eval(0), _tester(0), _ast(0),
    _builder(0)
{
    shell = new Shell(false);
}


void ASTExpressionEvaluatorTester::initTestCase()
{
    _specs = new MemSpecs();
    _specs->arch = MemSpecs::ar_x86_64;
    _specs->sizeofPointer = 8;
    _specs->sizeofLong = 8;

    _factory = new SymFactory(*_specs);

    // Create device from object dump above
    QByteArray ba(objdump);
    QBuffer buf(&ba);
    buf.open(QIODevice::ReadOnly);

    // Parse the object dump
    KernelSymbolParser parser(_factory);
    parser.parse(&buf);
}


void ASTExpressionEvaluatorTester::cleanupTestCase()
{
    safe_delete(_factory);
    safe_delete(_specs);
}


void ASTExpressionEvaluatorTester::init()
{
    _ast = new AbstractSyntaxTree();
    _builder = new ASTBuilder(_ast);
    _eval = new KernelSourceTypeEvaluator(_ast, _factory);
    _tester = new ASTExpressionTester(_ast, _factory, _eval);
}


void ASTExpressionEvaluatorTester::cleanup()
{
	if (!_ascii.isEmpty()) {
//		QString asciiStr = QString::fromAscii(_ascii.constData(), _ascii.size());
//		std::cerr << "--------[Failed program]--------" << std::endl
//				<< asciiStr.toStdString() << std::endl
//				<< "--------[/Failed program]--------" << std::endl;
		_ascii.clear();
	}

    safe_delete(_tester);
    safe_delete(_eval);
    safe_delete(_builder);
    safe_delete(_ast);
}

#define STRING2(s) #s
#define STRING(s) STRING2(s)
#define LN "line: " STRING(__LINE__)

enum ExpectFail {
    efType =   (1 << 0),
    efSize =   (1 << 1),
    efResult = (1 << 2)
};

#define CONSTANT_EXPR(expr) CONSTANT_EXPR3(expr, expr, 0)
#define CONSTANT_EXPR_FAIL(expr, fail) CONSTANT_EXPR3(expr, expr, fail)
#define CONSTANT_EXPR2(expr, expected) CONSTANT_EXPR3(expr, expected, 0)
#define CONSTANT_EXPR3(expr, expected, fail) \
    do { \
        if (exprSize(expected) & esInteger) \
            QTest::newRow(LN) << "int i = " #expr ";" \
                << (int)erConstant << (int)exprSize(expected) \
                << (quint64)((qint64)(expected)) << 0.0f << ((double) 0.0) \
                << (int)(fail); \
        else if (exprSize(expected) & esFloat) \
            QTest::newRow(LN) << "float f = " #expr ";" \
                << (int)erConstant << (int)exprSize(expected) \
                << 0ULL << (float)(expected) << ((double) 0.0) << (int)(fail); \
        else  \
            QTest::newRow(LN) << "int i = " #expr ";" \
                << (int)erConstant << (int)exprSize(expected) \
                << 0ULL << 0.0f << (double)(expected) << (int)(fail); \
    } while (0)

#define TEST_DATA_COLUMNS \
    QTest::addColumn<QString>("localCode"); \
    QTest::addColumn<int>("resultType"); \
    QTest::addColumn<int>("resultSize"); \
    QTest::addColumn<quint64>("result"); \
    QTest::addColumn<float>("resultf"); \
    QTest::addColumn<double>("resultd"); \
    QTest::addColumn<int>("expectFail"); \

#define	TEST_FUNCTION(methodName) \
	void ASTExpressionEvaluatorTester::test_##methodName##_func() \
	{ \
		QFETCH(QString, localCode); \
		QFETCH(int, resultType); \
		QFETCH(int, resultSize); \
		QFETCH(int, expectFail); \
		\
		_ascii += test_prog_begin; \
		if (!localCode.isEmpty()) { \
			_ascii += "    "; \
			_ascii += localCode.toAscii(); \
		} \
		\
		_ascii += test_prog_end; \
		\
		try{ \
			QCOMPARE(_builder->buildFrom(_ascii), 0); \
			QVERIFY(_tester->walkTree() > 0); \
		\
			if (expectFail & efType) \
				QEXPECT_FAIL("", "Expected wrong type", Continue); \
			QCOMPARE(_tester->result.resultType, resultType); \
			if (expectFail & efSize) \
				QEXPECT_FAIL("", "Expected wrong size", Continue); \
			QCOMPARE((int)_tester->result.size, resultSize); \
		\
			if (_tester->result.resultType == erConstant) { \
				if (expectFail & efResult) \
					QEXPECT_FAIL("", "Expected wrong result", Continue); \
				if (_tester->result.size & esInteger) { \
					QFETCH(quint64, result); \
					if (_tester->result.size & es64Bit) \
						QCOMPARE(_tester->result.result.ui64, result); \
					else if (_tester->result.size & es32Bit) \
						QCOMPARE(_tester->result.result.ui32, (quint32)result); \
					else if (_tester->result.size & es16Bit) \
						QCOMPARE(_tester->result.result.ui16, (quint16)result); \
					else \
						QCOMPARE(_tester->result.result.ui8, (quint8)result); \
				} \
				else if (_tester->result.size & esFloat) { \
					QFETCH(float, resultf); \
					QCOMPARE(_tester->result.result.f, resultf); \
				} \
				else if (_tester->result.size & esDouble) { \
					QFETCH(double, resultd); \
					QCOMPARE(_tester->result.result.d, resultd); \
				} \
				else { \
					QFAIL("Invalid result type"); \
				} \
			} \
		\
		} \
		catch (GenericException& e) { \
			/* Re-throw unexpected exceptions */ \
			std::cerr << "Caught exception at " << qPrintable(e.file) << ":" \
						<< e.line << ": " << qPrintable(e.message) << std::endl; \
			throw; \
		} \
		catch (...){ \
			cleanup(); \
			throw; \
		} \
		\
		_ascii.clear(); \
	} \
	\
   void ASTExpressionEvaluatorTester::test_##methodName##_func_data()


TEST_FUNCTION(constants)
{
    TEST_DATA_COLUMNS;

    // Positive constants
    CONSTANT_EXPR(0);
    CONSTANT_EXPR(1);
    CONSTANT_EXPR(1234567890);
    // Negative constants
    CONSTANT_EXPR(-1);
    CONSTANT_EXPR(-1234567890);
    // Constants with length specifieer
    CONSTANT_EXPR(99999L);
    CONSTANT_EXPR(99999UL);
    CONSTANT_EXPR(99999LU);
    CONSTANT_EXPR(99999LL);
    CONSTANT_EXPR(99999ULL);
    CONSTANT_EXPR(99999LLU);
    CONSTANT_EXPR(-99999L);
    CONSTANT_EXPR(-99999UL);
    CONSTANT_EXPR(-99999LU);
    CONSTANT_EXPR(-99999LL);
    CONSTANT_EXPR(-99999ULL);
    CONSTANT_EXPR(-99999LLU);
    // Character constants
    CONSTANT_EXPR('a');
    CONSTANT_EXPR('z');
    CONSTANT_EXPR('0');
    CONSTANT_EXPR(' ');
    CONSTANT_EXPR('\a');
    CONSTANT_EXPR('\r');
    CONSTANT_EXPR('\n');
    CONSTANT_EXPR('\t');
    CONSTANT_EXPR('\0');
    CONSTANT_EXPR('\001');
    CONSTANT_EXPR('\377');
    CONSTANT_EXPR('\xff');
    // Hex constants
    CONSTANT_EXPR(0xcafebabe);
    CONSTANT_EXPR(0xcafebabeUL);
    CONSTANT_EXPR(0xcafebabeLU);
    CONSTANT_EXPR(-0xcafebabe);
    CONSTANT_EXPR(-0xcafebabeL);
    CONSTANT_EXPR(-0xcafebabeUL);
    CONSTANT_EXPR(-0xcafebabeLU);
    // Octal constants
    CONSTANT_EXPR(01234567);
    CONSTANT_EXPR(01234567L);
    CONSTANT_EXPR(01234567UL);
    CONSTANT_EXPR(01234567LU);
    CONSTANT_EXPR(-01234567);
    CONSTANT_EXPR(-01234567L);
    CONSTANT_EXPR(-01234567UL);
    CONSTANT_EXPR(-01234567LU);
    // Float constants w/o exponent
    CONSTANT_EXPR(0.0);
    CONSTANT_EXPR(.0);
    CONSTANT_EXPR(1.0);
    CONSTANT_EXPR(-1.0);
    CONSTANT_EXPR(.34);
    CONSTANT_EXPR(12.34);
    CONSTANT_EXPR(-.34);
    CONSTANT_EXPR(-12.34);
    CONSTANT_EXPR(-1.0f);
    CONSTANT_EXPR(.34f);
    CONSTANT_EXPR(12.34f);
    CONSTANT_EXPR(-.34f);
    CONSTANT_EXPR(-12.34f);
    CONSTANT_EXPR((double) -1.0);
    CONSTANT_EXPR((double) .34);
    CONSTANT_EXPR((double) 12.34);
    CONSTANT_EXPR((double) -.34);
    CONSTANT_EXPR((double) -12.34);
    // Float constants w/ exponent
    CONSTANT_EXPR(0e0);
    CONSTANT_EXPR(0E0);
    CONSTANT_EXPR(1e0);
    CONSTANT_EXPR(1E0);
    CONSTANT_EXPR(-1e0);
    CONSTANT_EXPR(0e34);
    CONSTANT_EXPR(12e34);
    CONSTANT_EXPR(-0e34);
    CONSTANT_EXPR(-12e34);
    CONSTANT_EXPR(-12E34);
    CONSTANT_EXPR(12e34f);
    CONSTANT_EXPR(-0e34f);
    CONSTANT_EXPR(-12e34f);
    CONSTANT_EXPR(-12E34f);
    CONSTANT_EXPR((double) 12e34);
    CONSTANT_EXPR((double) -0e34);
    CONSTANT_EXPR((double) -12e34);
    CONSTANT_EXPR((double) -12E34);
}


TEST_FUNCTION(sign_extension)
{
    TEST_DATA_COLUMNS;

    // Bitwise expressions
    CONSTANT_EXPR('1' ^ 0xcafe);
    CONSTANT_EXPR('1' ^ 0xcafeU);
    CONSTANT_EXPR('1' ^ 0xcafeLL);
    CONSTANT_EXPR('1' ^ 0xcafeULL);

    CONSTANT_EXPR(1 ^ 0xcafe);
    CONSTANT_EXPR(1 ^ 0xcafeU);
    CONSTANT_EXPR(1 ^ 0xcafeLL);
    CONSTANT_EXPR(1 ^ 0xcafeULL);

    CONSTANT_EXPR(1U ^ 0xcafe);
    CONSTANT_EXPR(1U ^ 0xcafeU);
    CONSTANT_EXPR(1U ^ 0xcafeLL);
    CONSTANT_EXPR(1U ^ 0xcafeULL);

    CONSTANT_EXPR(1LL ^ 0xcafe);
    CONSTANT_EXPR(1LL ^ 0xcafeU);
    CONSTANT_EXPR(1LL ^ 0xcafeLL);
    CONSTANT_EXPR(1LL ^ 0xcafeULL);

    CONSTANT_EXPR(1ULL ^ 0xcafe);
    CONSTANT_EXPR(1ULL ^ 0xcafeU);
    CONSTANT_EXPR(1ULL ^ 0xcafeLL);
    CONSTANT_EXPR(1ULL ^ 0xcafeULL);

    CONSTANT_EXPR(-'1' ^ 0xcafe);
    CONSTANT_EXPR(-'1' ^ 0xcafeU);
    CONSTANT_EXPR(-'1' ^ 0xcafeLL);
    CONSTANT_EXPR(-'1' ^ 0xcafeULL);

    CONSTANT_EXPR(-1 ^ 0xcafe);
    CONSTANT_EXPR(-1 ^ 0xcafeU);
    CONSTANT_EXPR(-1 ^ 0xcafeLL);
    CONSTANT_EXPR(-1 ^ 0xcafeULL);

    CONSTANT_EXPR(-1U ^ 0xcafe);
    CONSTANT_EXPR(-1U ^ 0xcafeU);
    CONSTANT_EXPR(-1U ^ 0xcafeLL);
    CONSTANT_EXPR(-1U ^ 0xcafeULL);

    CONSTANT_EXPR(-1LL ^ 0xcafe);
    CONSTANT_EXPR(-1LL ^ 0xcafeU);
    CONSTANT_EXPR(-1LL ^ 0xcafeLL);
    CONSTANT_EXPR(-1LL ^ 0xcafeULL);

    CONSTANT_EXPR(-1ULL ^ 0xcafe);
    CONSTANT_EXPR(-1ULL ^ 0xcafeU);
    CONSTANT_EXPR(-1ULL ^ 0xcafeLL);
    CONSTANT_EXPR(-1ULL ^ 0xcafeULL);

    CONSTANT_EXPR('\1' ^ -0xcafe);
    CONSTANT_EXPR('\1' ^ -0xcafeU);
    CONSTANT_EXPR('\1' ^ -0xcafeLL);
    CONSTANT_EXPR('\1' ^ -0xcafeULL);

    CONSTANT_EXPR(1 ^ -0xcafe);
    CONSTANT_EXPR(1 ^ -0xcafeU);
    CONSTANT_EXPR(1 ^ -0xcafeLL);
    CONSTANT_EXPR(1 ^ -0xcafeULL);

    CONSTANT_EXPR(1U ^ -0xcafe);
    CONSTANT_EXPR(1U ^ -0xcafeU);
    CONSTANT_EXPR(1U ^ -0xcafeLL);
    CONSTANT_EXPR(1U ^ -0xcafeULL);

    CONSTANT_EXPR(1LL ^ -0xcafe);
    CONSTANT_EXPR(1LL ^ -0xcafeU);
    CONSTANT_EXPR(1LL ^ -0xcafeLL);
    CONSTANT_EXPR(1LL ^ -0xcafeULL);

    CONSTANT_EXPR(1ULL ^ -0xcafe);
    CONSTANT_EXPR(1ULL ^ -0xcafeU);
    CONSTANT_EXPR(1ULL ^ -0xcafeLL);
    CONSTANT_EXPR(1ULL ^ -0xcafeULL);

    CONSTANT_EXPR(-'\1' ^ -0xcafe);
    CONSTANT_EXPR(-'\1' ^ -0xcafeU);
    CONSTANT_EXPR(-'\1' ^ -0xcafeLL);
    CONSTANT_EXPR(-'\1' ^ -0xcafeULL);

    CONSTANT_EXPR(-1 ^ -0xcafe);
    CONSTANT_EXPR(-1 ^ -0xcafeU);
    CONSTANT_EXPR(-1 ^ -0xcafeLL);
    CONSTANT_EXPR(-1 ^ -0xcafeULL);

    CONSTANT_EXPR(-1U ^ -0xcafe);
    CONSTANT_EXPR(-1U ^ -0xcafeU);
    CONSTANT_EXPR(-1U ^ -0xcafeLL);
    CONSTANT_EXPR(-1U ^ -0xcafeULL);

    CONSTANT_EXPR(-1LL ^ -0xcafe);
    CONSTANT_EXPR(-1LL ^ -0xcafeU);
    CONSTANT_EXPR(-1LL ^ -0xcafeLL);
    CONSTANT_EXPR(-1LL ^ -0xcafeULL);

    CONSTANT_EXPR(-1ULL ^ -0xcafe);
    CONSTANT_EXPR(-1ULL ^ -0xcafeU);
    CONSTANT_EXPR(-1ULL ^ -0xcafeLL);
    CONSTANT_EXPR(-1ULL ^ -0xcafeULL);

    CONSTANT_EXPR('\1' ^ 0xcafebabe);
    CONSTANT_EXPR('\1' ^ 0xcafebabeU);
    CONSTANT_EXPR('\1' ^ 0xcafebabeLL);
    CONSTANT_EXPR('\1' ^ 0xcafebabeULL);

    CONSTANT_EXPR(1 ^ 0xcafebabe);
    CONSTANT_EXPR(1 ^ 0xcafebabeU);
    CONSTANT_EXPR(1 ^ 0xcafebabeLL);
    CONSTANT_EXPR(1 ^ 0xcafebabeULL);

    CONSTANT_EXPR(1U ^ 0xcafebabe);
    CONSTANT_EXPR(1U ^ 0xcafebabeU);
    CONSTANT_EXPR(1U ^ 0xcafebabeLL);
    CONSTANT_EXPR(1U ^ 0xcafebabeULL);

    CONSTANT_EXPR(1LL ^ 0xcafebabe);
    CONSTANT_EXPR(1LL ^ 0xcafebabeU);
    CONSTANT_EXPR(1LL ^ 0xcafebabeLL);
    CONSTANT_EXPR(1LL ^ 0xcafebabeULL);

    CONSTANT_EXPR(1ULL ^ 0xcafebabe);
    CONSTANT_EXPR(1ULL ^ 0xcafebabeU);
    CONSTANT_EXPR(1ULL ^ 0xcafebabeLL);
    CONSTANT_EXPR(1ULL ^ 0xcafebabeULL);

    CONSTANT_EXPR(-'\1' ^ 0xcafebabe);
    CONSTANT_EXPR(-'\1' ^ 0xcafebabeU);
    CONSTANT_EXPR(-'\1' ^ 0xcafebabeLL);
    CONSTANT_EXPR(-'\1' ^ 0xcafebabeULL);

    CONSTANT_EXPR(-1 ^ 0xcafebabe);
    CONSTANT_EXPR(-1 ^ 0xcafebabeU);
    CONSTANT_EXPR(-1 ^ 0xcafebabeLL);
    CONSTANT_EXPR(-1 ^ 0xcafebabeULL);

    CONSTANT_EXPR(-1U ^ 0xcafebabe);
    CONSTANT_EXPR(-1U ^ 0xcafebabeU);
    CONSTANT_EXPR(-1U ^ 0xcafebabeLL);
    CONSTANT_EXPR(-1U ^ 0xcafebabeULL);

    CONSTANT_EXPR(-1LL ^ 0xcafebabe);
    CONSTANT_EXPR(-1LL ^ 0xcafebabeU);
    CONSTANT_EXPR(-1LL ^ 0xcafebabeLL);
    CONSTANT_EXPR(-1LL ^ 0xcafebabeULL);

    CONSTANT_EXPR(-1ULL ^ 0xcafebabe);
    CONSTANT_EXPR(-1ULL ^ 0xcafebabeU);
    CONSTANT_EXPR(-1ULL ^ 0xcafebabeLL);
    CONSTANT_EXPR(-1ULL ^ 0xcafebabeULL);

    CONSTANT_EXPR('\1' ^ -0xcafebabe);
    CONSTANT_EXPR('\1' ^ -0xcafebabeU);
    CONSTANT_EXPR('\1' ^ -0xcafebabeLL);
    CONSTANT_EXPR('\1' ^ -0xcafebabeULL);

    CONSTANT_EXPR(1 ^ -0xcafebabe);
    CONSTANT_EXPR(1 ^ -0xcafebabeU);
    CONSTANT_EXPR(1 ^ -0xcafebabeLL);
    CONSTANT_EXPR(1 ^ -0xcafebabeULL);

    CONSTANT_EXPR(1U ^ -0xcafebabe);
    CONSTANT_EXPR(1U ^ -0xcafebabeU);
    CONSTANT_EXPR(1U ^ -0xcafebabeLL);
    CONSTANT_EXPR(1U ^ -0xcafebabeULL);

    CONSTANT_EXPR(1LL ^ -0xcafebabe);
    CONSTANT_EXPR(1LL ^ -0xcafebabeU);
    CONSTANT_EXPR(1LL ^ -0xcafebabeLL);
    CONSTANT_EXPR(1LL ^ -0xcafebabeULL);

    CONSTANT_EXPR(1ULL ^ -0xcafebabe);
    CONSTANT_EXPR(1ULL ^ -0xcafebabeU);
    CONSTANT_EXPR(1ULL ^ -0xcafebabeLL);
    CONSTANT_EXPR(1ULL ^ -0xcafebabeULL);

    CONSTANT_EXPR(-'\1' ^ -0xcafebabe);
    CONSTANT_EXPR(-'\1' ^ -0xcafebabeU);
    CONSTANT_EXPR(-'\1' ^ -0xcafebabeLL);
    CONSTANT_EXPR(-'\1' ^ -0xcafebabeULL);

    CONSTANT_EXPR(-1 ^ -0xcafebabe);
    CONSTANT_EXPR(-1 ^ -0xcafebabeU);
    CONSTANT_EXPR(-1 ^ -0xcafebabeLL);
    CONSTANT_EXPR(-1 ^ -0xcafebabeULL);

    CONSTANT_EXPR(-1U ^ -0xcafebabe);
    CONSTANT_EXPR(-1U ^ -0xcafebabeU);
    CONSTANT_EXPR(-1U ^ -0xcafebabeLL);
    CONSTANT_EXPR(-1U ^ -0xcafebabeULL);

    CONSTANT_EXPR(-1LL ^ -0xcafebabe);
    CONSTANT_EXPR(-1LL ^ -0xcafebabeU);
    CONSTANT_EXPR(-1LL ^ -0xcafebabeLL);
    CONSTANT_EXPR(-1LL ^ -0xcafebabeULL);

    CONSTANT_EXPR(-1ULL ^ -0xcafebabe);
    CONSTANT_EXPR(-1ULL ^ -0xcafebabeU);
    CONSTANT_EXPR(-1ULL ^ -0xcafebabeLL);
    CONSTANT_EXPR(-1ULL ^ -0xcafebabeULL);

    // Logical expressions
    CONSTANT_EXPR('\1' > 0xcafe);
    CONSTANT_EXPR('\1' > 0xcafeU);
    CONSTANT_EXPR('\1' > 0xcafeLL);
    CONSTANT_EXPR('\1' > 0xcafeULL);

    CONSTANT_EXPR(1 > 0xcafe);
    CONSTANT_EXPR(1 > 0xcafeU);
    CONSTANT_EXPR(1 > 0xcafeLL);
    CONSTANT_EXPR(1 > 0xcafeULL);

    CONSTANT_EXPR(1U > 0xcafe);
    CONSTANT_EXPR(1U > 0xcafeU);
    CONSTANT_EXPR(1U > 0xcafeLL);
    CONSTANT_EXPR(1U > 0xcafeULL);

    CONSTANT_EXPR(1LL > 0xcafe);
    CONSTANT_EXPR(1LL > 0xcafeU);
    CONSTANT_EXPR(1LL > 0xcafeLL);
    CONSTANT_EXPR(1LL > 0xcafeULL);

    CONSTANT_EXPR(1ULL > 0xcafe);
    CONSTANT_EXPR(1ULL > 0xcafeU);
    CONSTANT_EXPR(1ULL > 0xcafeLL);
    CONSTANT_EXPR(1ULL > 0xcafeULL);

    CONSTANT_EXPR(-'\1' > 0xcafe);
    CONSTANT_EXPR(-'\1' > 0xcafeU);
    CONSTANT_EXPR(-'\1' > 0xcafeLL);
    CONSTANT_EXPR(-'\1' > 0xcafeULL);

    CONSTANT_EXPR(-1 > 0xcafe);
    CONSTANT_EXPR(-1 > 0xcafeU);
    CONSTANT_EXPR(-1 > 0xcafeLL);
    CONSTANT_EXPR(-1 > 0xcafeULL);

    CONSTANT_EXPR(-1U > 0xcafe);
    CONSTANT_EXPR(-1U > 0xcafeU);
    CONSTANT_EXPR(-1U > 0xcafeLL);
    CONSTANT_EXPR(-1U > 0xcafeULL);

    CONSTANT_EXPR(-1LL > 0xcafe);
    CONSTANT_EXPR(-1LL > 0xcafeU);
    CONSTANT_EXPR(-1LL > 0xcafeLL);
    CONSTANT_EXPR(-1LL > 0xcafeULL);

    CONSTANT_EXPR(-1ULL > 0xcafe);
    CONSTANT_EXPR(-1ULL > 0xcafeU);
    CONSTANT_EXPR(-1ULL > 0xcafeLL);
    CONSTANT_EXPR(-1ULL > 0xcafeULL);

    CONSTANT_EXPR('\1' > -0xcafe);
    CONSTANT_EXPR('\1' > -0xcafeU);
    CONSTANT_EXPR('\1' > -0xcafeLL);
    CONSTANT_EXPR('\1' > -0xcafeULL);

    CONSTANT_EXPR(1 > -0xcafe);
    CONSTANT_EXPR(1 > -0xcafeU);
    CONSTANT_EXPR(1 > -0xcafeLL);
    CONSTANT_EXPR(1 > -0xcafeULL);

    CONSTANT_EXPR(1U > -0xcafe);
    CONSTANT_EXPR(1U > -0xcafeU);
    CONSTANT_EXPR(1U > -0xcafeLL);
    CONSTANT_EXPR(1U > -0xcafeULL);

    CONSTANT_EXPR(1LL > -0xcafe);
    CONSTANT_EXPR(1LL > -0xcafeU);
    CONSTANT_EXPR(1LL > -0xcafeLL);
    CONSTANT_EXPR(1LL > -0xcafeULL);

    CONSTANT_EXPR(1ULL > -0xcafe);
    CONSTANT_EXPR(1ULL > -0xcafeU);
    CONSTANT_EXPR(1ULL > -0xcafeLL);
    CONSTANT_EXPR(1ULL > -0xcafeULL);

    CONSTANT_EXPR(-'\1' > -0xcafe);
    CONSTANT_EXPR(-'\1' > -0xcafeU);
    CONSTANT_EXPR(-'\1' > -0xcafeLL);
    CONSTANT_EXPR(-'\1' > -0xcafeULL);

    CONSTANT_EXPR(-1 > -0xcafe);
    CONSTANT_EXPR(-1 > -0xcafeU);
    CONSTANT_EXPR(-1 > -0xcafeLL);
    CONSTANT_EXPR(-1 > -0xcafeULL);

    CONSTANT_EXPR(-1U > -0xcafe);
    CONSTANT_EXPR(-1U > -0xcafeU);
    CONSTANT_EXPR(-1U > -0xcafeLL);
    CONSTANT_EXPR(-1U > -0xcafeULL);

    CONSTANT_EXPR(-1LL > -0xcafe);
    CONSTANT_EXPR(-1LL > -0xcafeU);
    CONSTANT_EXPR(-1LL > -0xcafeLL);
    CONSTANT_EXPR(-1LL > -0xcafeULL);

    CONSTANT_EXPR(-1ULL > -0xcafe);
    CONSTANT_EXPR(-1ULL > -0xcafeU);
    CONSTANT_EXPR(-1ULL > -0xcafeLL);
    CONSTANT_EXPR(-1ULL > -0xcafeULL);

    CONSTANT_EXPR('\1' > 0xcafebabe);
    CONSTANT_EXPR('\1' > 0xcafebabeU);
    CONSTANT_EXPR('\1' > 0xcafebabeLL);
    CONSTANT_EXPR('\1' > 0xcafebabeULL);

    CONSTANT_EXPR(1 > 0xcafebabe);
    CONSTANT_EXPR(1 > 0xcafebabeU);
    CONSTANT_EXPR(1 > 0xcafebabeLL);
    CONSTANT_EXPR(1 > 0xcafebabeULL);

    CONSTANT_EXPR(1U > 0xcafebabe);
    CONSTANT_EXPR(1U > 0xcafebabeU);
    CONSTANT_EXPR(1U > 0xcafebabeLL);
    CONSTANT_EXPR(1U > 0xcafebabeULL);

    CONSTANT_EXPR(1LL > 0xcafebabe);
    CONSTANT_EXPR(1LL > 0xcafebabeU);
    CONSTANT_EXPR(1LL > 0xcafebabeLL);
    CONSTANT_EXPR(1LL > 0xcafebabeULL);

    CONSTANT_EXPR(1ULL > 0xcafebabe);
    CONSTANT_EXPR(1ULL > 0xcafebabeU);
    CONSTANT_EXPR(1ULL > 0xcafebabeLL);
    CONSTANT_EXPR(1ULL > 0xcafebabeULL);

    CONSTANT_EXPR(-'\1' > 0xcafebabe);
    CONSTANT_EXPR(-'\1' > 0xcafebabeU);
    CONSTANT_EXPR(-'\1' > 0xcafebabeLL);
    CONSTANT_EXPR(-'\1' > 0xcafebabeULL);

    CONSTANT_EXPR(-1 > 0xcafebabe);
    CONSTANT_EXPR(-1 > 0xcafebabeU);
    CONSTANT_EXPR(-1 > 0xcafebabeLL);
    CONSTANT_EXPR(-1 > 0xcafebabeULL);

    CONSTANT_EXPR(-1U > 0xcafebabe);
    CONSTANT_EXPR(-1U > 0xcafebabeU);
    CONSTANT_EXPR(-1U > 0xcafebabeLL);
    CONSTANT_EXPR(-1U > 0xcafebabeULL);

    CONSTANT_EXPR(-1LL > 0xcafebabe);
    CONSTANT_EXPR(-1LL > 0xcafebabeU);
    CONSTANT_EXPR(-1LL > 0xcafebabeLL);
    CONSTANT_EXPR(-1LL > 0xcafebabeULL);

    CONSTANT_EXPR(-1ULL > 0xcafebabe);
    CONSTANT_EXPR(-1ULL > 0xcafebabeU);
    CONSTANT_EXPR(-1ULL > 0xcafebabeLL);
    CONSTANT_EXPR(-1ULL > 0xcafebabeULL);

    CONSTANT_EXPR('\1' > -0xcafebabe);
    CONSTANT_EXPR('\1' > -0xcafebabeU);
    CONSTANT_EXPR('\1' > -0xcafebabeLL);
    CONSTANT_EXPR('\1' > -0xcafebabeULL);

    CONSTANT_EXPR(1 > -0xcafebabe);
    CONSTANT_EXPR(1 > -0xcafebabeU);
    CONSTANT_EXPR(1 > -0xcafebabeLL);
    CONSTANT_EXPR(1 > -0xcafebabeULL);

    CONSTANT_EXPR(1U > -0xcafebabe);
    CONSTANT_EXPR(1U > -0xcafebabeU);
    CONSTANT_EXPR(1U > -0xcafebabeLL);
    CONSTANT_EXPR(1U > -0xcafebabeULL);

    CONSTANT_EXPR(1LL > -0xcafebabe);
    CONSTANT_EXPR(1LL > -0xcafebabeU);
    CONSTANT_EXPR(1LL > -0xcafebabeLL);
    CONSTANT_EXPR(1LL > -0xcafebabeULL);

    CONSTANT_EXPR(1ULL > -0xcafebabe);
    CONSTANT_EXPR(1ULL > -0xcafebabeU);
    CONSTANT_EXPR(1ULL > -0xcafebabeLL);
    CONSTANT_EXPR(1ULL > -0xcafebabeULL);

    CONSTANT_EXPR(-'\1' > -0xcafebabe);
    CONSTANT_EXPR(-'\1' > -0xcafebabeU);
    CONSTANT_EXPR(-'\1' > -0xcafebabeLL);
    CONSTANT_EXPR(-'\1' > -0xcafebabeULL);

    CONSTANT_EXPR(-1 > -0xcafebabe);
    CONSTANT_EXPR(-1 > -0xcafebabeU);
    CONSTANT_EXPR(-1 > -0xcafebabeLL);
    CONSTANT_EXPR(-1 > -0xcafebabeULL);

    CONSTANT_EXPR(-1U > -0xcafebabe);
    CONSTANT_EXPR(-1U > -0xcafebabeU);
    CONSTANT_EXPR(-1U > -0xcafebabeLL);
    CONSTANT_EXPR(-1U > -0xcafebabeULL);

    CONSTANT_EXPR(-1LL > -0xcafebabe);
    CONSTANT_EXPR(-1LL > -0xcafebabeU);
    CONSTANT_EXPR(-1LL > -0xcafebabeLL);
    CONSTANT_EXPR(-1LL > -0xcafebabeULL);

    CONSTANT_EXPR(-1ULL > -0xcafebabe);
    CONSTANT_EXPR(-1ULL > -0xcafebabeU);
    CONSTANT_EXPR(-1ULL > -0xcafebabeLL);
    CONSTANT_EXPR(-1ULL > -0xcafebabeULL);

    // Comparisons
    CONSTANT_EXPR(1 && 0xcafe);
    CONSTANT_EXPR(1 && 0xcafeU);
    CONSTANT_EXPR(1 && 0xcafeLL);
    CONSTANT_EXPR(1 && 0xcafeULL);

    CONSTANT_EXPR(1U && 0xcafe);
    CONSTANT_EXPR(1U && 0xcafeU);
    CONSTANT_EXPR(1U && 0xcafeLL);
    CONSTANT_EXPR(1U && 0xcafeULL);

    CONSTANT_EXPR(1LL && 0xcafe);
    CONSTANT_EXPR(1LL && 0xcafeU);
    CONSTANT_EXPR(1LL && 0xcafeLL);
    CONSTANT_EXPR(1LL && 0xcafeULL);

    CONSTANT_EXPR(1ULL && 0xcafe);
    CONSTANT_EXPR(1ULL && 0xcafeU);
    CONSTANT_EXPR(1ULL && 0xcafeLL);
    CONSTANT_EXPR(1ULL && 0xcafeULL);

    CONSTANT_EXPR(-1 && 0xcafe);
    CONSTANT_EXPR(-1 && 0xcafeU);
    CONSTANT_EXPR(-1 && 0xcafeLL);
    CONSTANT_EXPR(-1 && 0xcafeULL);

    CONSTANT_EXPR(-1U && 0xcafe);
    CONSTANT_EXPR(-1U && 0xcafeU);
    CONSTANT_EXPR(-1U && 0xcafeLL);
    CONSTANT_EXPR(-1U && 0xcafeULL);

    CONSTANT_EXPR(-1LL && 0xcafe);
    CONSTANT_EXPR(-1LL && 0xcafeU);
    CONSTANT_EXPR(-1LL && 0xcafeLL);
    CONSTANT_EXPR(-1LL && 0xcafeULL);

    CONSTANT_EXPR(-1ULL && 0xcafe);
    CONSTANT_EXPR(-1ULL && 0xcafeU);
    CONSTANT_EXPR(-1ULL && 0xcafeLL);
    CONSTANT_EXPR(-1ULL && 0xcafeULL);

    CONSTANT_EXPR(1 && -0xcafe);
    CONSTANT_EXPR(1 && -0xcafeU);
    CONSTANT_EXPR(1 && -0xcafeLL);
    CONSTANT_EXPR(1 && -0xcafeULL);

    CONSTANT_EXPR(1U && -0xcafe);
    CONSTANT_EXPR(1U && -0xcafeU);
    CONSTANT_EXPR(1U && -0xcafeLL);
    CONSTANT_EXPR(1U && -0xcafeULL);

    CONSTANT_EXPR(1LL && -0xcafe);
    CONSTANT_EXPR(1LL && -0xcafeU);
    CONSTANT_EXPR(1LL && -0xcafeLL);
    CONSTANT_EXPR(1LL && -0xcafeULL);

    CONSTANT_EXPR(1ULL && -0xcafe);
    CONSTANT_EXPR(1ULL && -0xcafeU);
    CONSTANT_EXPR(1ULL && -0xcafeLL);
    CONSTANT_EXPR(1ULL && -0xcafeULL);

    CONSTANT_EXPR(-1 && -0xcafe);
    CONSTANT_EXPR(-1 && -0xcafeU);
    CONSTANT_EXPR(-1 && -0xcafeLL);
    CONSTANT_EXPR(-1 && -0xcafeULL);

    CONSTANT_EXPR(-1U && -0xcafe);
    CONSTANT_EXPR(-1U && -0xcafeU);
    CONSTANT_EXPR(-1U && -0xcafeLL);
    CONSTANT_EXPR(-1U && -0xcafeULL);

    CONSTANT_EXPR(-1LL && -0xcafe);
    CONSTANT_EXPR(-1LL && -0xcafeU);
    CONSTANT_EXPR(-1LL && -0xcafeLL);
    CONSTANT_EXPR(-1LL && -0xcafeULL);

    CONSTANT_EXPR(-1ULL && -0xcafe);
    CONSTANT_EXPR(-1ULL && -0xcafeU);
    CONSTANT_EXPR(-1ULL && -0xcafeLL);
    CONSTANT_EXPR(-1ULL && -0xcafeULL);

    CONSTANT_EXPR(1 && 0xcafebabe);
    CONSTANT_EXPR(1 && 0xcafebabeU);
    CONSTANT_EXPR(1 && 0xcafebabeLL);
    CONSTANT_EXPR(1 && 0xcafebabeULL);

    CONSTANT_EXPR(1U && 0xcafebabe);
    CONSTANT_EXPR(1U && 0xcafebabeU);
    CONSTANT_EXPR(1U && 0xcafebabeLL);
    CONSTANT_EXPR(1U && 0xcafebabeULL);

    CONSTANT_EXPR(1LL && 0xcafebabe);
    CONSTANT_EXPR(1LL && 0xcafebabeU);
    CONSTANT_EXPR(1LL && 0xcafebabeLL);
    CONSTANT_EXPR(1LL && 0xcafebabeULL);

    CONSTANT_EXPR(1ULL && 0xcafebabe);
    CONSTANT_EXPR(1ULL && 0xcafebabeU);
    CONSTANT_EXPR(1ULL && 0xcafebabeLL);
    CONSTANT_EXPR(1ULL && 0xcafebabeULL);

    CONSTANT_EXPR(-1 && 0xcafebabe);
    CONSTANT_EXPR(-1 && 0xcafebabeU);
    CONSTANT_EXPR(-1 && 0xcafebabeLL);
    CONSTANT_EXPR(-1 && 0xcafebabeULL);

    CONSTANT_EXPR(-1U && 0xcafebabe);
    CONSTANT_EXPR(-1U && 0xcafebabeU);
    CONSTANT_EXPR(-1U && 0xcafebabeLL);
    CONSTANT_EXPR(-1U && 0xcafebabeULL);

    CONSTANT_EXPR(-1LL && 0xcafebabe);
    CONSTANT_EXPR(-1LL && 0xcafebabeU);
    CONSTANT_EXPR(-1LL && 0xcafebabeLL);
    CONSTANT_EXPR(-1LL && 0xcafebabeULL);

    CONSTANT_EXPR(-1ULL && 0xcafebabe);
    CONSTANT_EXPR(-1ULL && 0xcafebabeU);
    CONSTANT_EXPR(-1ULL && 0xcafebabeLL);
    CONSTANT_EXPR(-1ULL && 0xcafebabeULL);

    CONSTANT_EXPR(1 && -0xcafebabe);
    CONSTANT_EXPR(1 && -0xcafebabeU);
    CONSTANT_EXPR(1 && -0xcafebabeLL);
    CONSTANT_EXPR(1 && -0xcafebabeULL);

    CONSTANT_EXPR(1U && -0xcafebabe);
    CONSTANT_EXPR(1U && -0xcafebabeU);
    CONSTANT_EXPR(1U && -0xcafebabeLL);
    CONSTANT_EXPR(1U && -0xcafebabeULL);

    CONSTANT_EXPR(1LL && -0xcafebabe);
    CONSTANT_EXPR(1LL && -0xcafebabeU);
    CONSTANT_EXPR(1LL && -0xcafebabeLL);
    CONSTANT_EXPR(1LL && -0xcafebabeULL);

    CONSTANT_EXPR(1ULL && -0xcafebabe);
    CONSTANT_EXPR(1ULL && -0xcafebabeU);
    CONSTANT_EXPR(1ULL && -0xcafebabeLL);
    CONSTANT_EXPR(1ULL && -0xcafebabeULL);

    CONSTANT_EXPR(-1 && -0xcafebabe);
    CONSTANT_EXPR(-1 && -0xcafebabeU);
    CONSTANT_EXPR(-1 && -0xcafebabeLL);
    CONSTANT_EXPR(-1 && -0xcafebabeULL);

    CONSTANT_EXPR(-1U && -0xcafebabe);
    CONSTANT_EXPR(-1U && -0xcafebabeU);
    CONSTANT_EXPR(-1U && -0xcafebabeLL);
    CONSTANT_EXPR(-1U && -0xcafebabeULL);

    CONSTANT_EXPR(-1LL && -0xcafebabe);
    CONSTANT_EXPR(-1LL && -0xcafebabeU);
    CONSTANT_EXPR(-1LL && -0xcafebabeLL);
    CONSTANT_EXPR(-1LL && -0xcafebabeULL);

    CONSTANT_EXPR(-1ULL && -0xcafebabe);
    CONSTANT_EXPR(-1ULL && -0xcafebabeU);
    CONSTANT_EXPR(-1ULL && -0xcafebabeLL);
    CONSTANT_EXPR(-1ULL && -0xcafebabeULL);
}


TEST_FUNCTION(additive)
{
    TEST_DATA_COLUMNS;

    // Integers
    CONSTANT_EXPR(1 + 1);
    CONSTANT_EXPR(2 + 1);
    CONSTANT_EXPR(2 - 1);
    CONSTANT_EXPR(2 + -1);
    CONSTANT_EXPR((2) + (-1));
    CONSTANT_EXPR(-1 + 2);
    CONSTANT_EXPR((-1) + (2));
    CONSTANT_EXPR(-1 + 2 - 3);
    CONSTANT_EXPR(1 - 2 + 3);
    CONSTANT_EXPR((1 - 2) + 3);
    CONSTANT_EXPR(1 - (2 + 3));
    CONSTANT_EXPR(1 + (-2 + 3));
    CONSTANT_EXPR(1 + -(-(2 + 3) - 4 + -(5 - 6)));

    // Mixed integer sizes and signedness
    CONSTANT_EXPR(1U + (-2 + '\3'));
    CONSTANT_EXPR('\1' + (-2U + 3));
    CONSTANT_EXPR('\1' + (-2 + 3U));
    CONSTANT_EXPR(1L + (-'\2' + 3));
    CONSTANT_EXPR('\1' + (-2L + 3));
    CONSTANT_EXPR('\1' + (-2 + 3L));
    CONSTANT_EXPR(1UL + (-2 + '\3'));
    CONSTANT_EXPR('\1' + (-2UL + 3));
    CONSTANT_EXPR('\1' + (-2 + 3UL));
    CONSTANT_EXPR(1LL + (-'\2' + 3));
    CONSTANT_EXPR(1 + (-2LL + '\3'));
    CONSTANT_EXPR(1 + (-'\2' + 3LL));
    CONSTANT_EXPR(1ULL + (-2 + '\3'));
    CONSTANT_EXPR(1 + (-2ULL + '\3'));
    CONSTANT_EXPR('\1' + (-2 + 3ULL));
    CONSTANT_EXPR(1L + (-2UL + 3U));
    CONSTANT_EXPR(1U + (-2L + 3UL));
    CONSTANT_EXPR(1UL + (-2U + 3L));

    // Floats and integers mixed
    CONSTANT_EXPR('\1' + 2.2);
    CONSTANT_EXPR(1 + 2.2);
    CONSTANT_EXPR(1.1 + 2);
    CONSTANT_EXPR(1U + 2.2);
    CONSTANT_EXPR(1.1 + 2U);
    CONSTANT_EXPR(1L + 2.2);
    CONSTANT_EXPR(1.1 + 2L);
    CONSTANT_EXPR(1UL + 2.2);
    CONSTANT_EXPR(1.1 + 2UL);

    CONSTANT_EXPR('\1' + 2.2f);
    CONSTANT_EXPR(1 + 2.2f);
    CONSTANT_EXPR(1.1f + 2);
    CONSTANT_EXPR(1U + 2.2f);
    CONSTANT_EXPR(1.1f + 2U);
    CONSTANT_EXPR(1L + 2.2f);
    CONSTANT_EXPR(1.1f + 2L);
    CONSTANT_EXPR(1UL + 2.2f);
    CONSTANT_EXPR(1.1f + 2UL);

    CONSTANT_EXPR('\1' + ((double) 2.2));
    CONSTANT_EXPR(1 + ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) + 2);
    CONSTANT_EXPR(1U + ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) + 2U);
    CONSTANT_EXPR(1L + ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) + 2L);
    CONSTANT_EXPR(1UL + ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) + 2UL);

    // Floats mixed
    CONSTANT_EXPR(1.1 + 2.2);
    CONSTANT_EXPR(1.1 + 2.2f);
    CONSTANT_EXPR(1.1 + ((double) 2.2));

    CONSTANT_EXPR(1.1f + 2.2);
    CONSTANT_EXPR(1.1f + 2.2f);
    CONSTANT_EXPR(1.1f + ((double) 2.2));

    CONSTANT_EXPR(((double) 1.1) + 2.2);
    CONSTANT_EXPR(((double) 1.1) + 2.2f);
    CONSTANT_EXPR(((double) 1.1) + ((double) 2.2));

    // All in the mix
    CONSTANT_EXPR(1 + -(-(2l + 3u) - 4ul + -(5ll - 6ull) + 7.654f + -(8.76) - ((double) 9.876)));
}


TEST_FUNCTION(multiplicative)
{
    TEST_DATA_COLUMNS;

    // Integers
    CONSTANT_EXPR(5 * 5);
    CONSTANT_EXPR(2 * 5);
    CONSTANT_EXPR(2 / 1);
    CONSTANT_EXPR(5 % 2);
    CONSTANT_EXPR(-5 % 2);
    CONSTANT_EXPR(5 % -2);
    CONSTANT_EXPR(-5 % -2);
    CONSTANT_EXPR(2 * -1);
    CONSTANT_EXPR((2) * (-1));
    CONSTANT_EXPR(-1 * 2);
    CONSTANT_EXPR((-1) * (2));
    CONSTANT_EXPR(-1 * 2 / 3);
    CONSTANT_EXPR(5 % 7 / 3);
    CONSTANT_EXPR(5 / 2 * 3);
    CONSTANT_EXPR(5 / 2 % 3);
    CONSTANT_EXPR((5 / 2) * 3);
    CONSTANT_EXPR(5 / (2 * 3));
    CONSTANT_EXPR((5 % 2) * 3);
    CONSTANT_EXPR(5 / (2 % 3));
    CONSTANT_EXPR(1 * (-2 * 3));
    CONSTANT_EXPR((4 * -(5 / 6)) % 1 * -(-(2 * 3)));

    // Mixed integer sizes and signedness
    CONSTANT_EXPR(1U * (-2 * 3));
    CONSTANT_EXPR(1 * (-2U * 3));
    CONSTANT_EXPR(1 * (-2 * 3U));
    CONSTANT_EXPR(1L * (-2 * 3));
    CONSTANT_EXPR(1 * (-2L * 3));
    CONSTANT_EXPR(1 * (-2 * 3L));
    CONSTANT_EXPR(1UL * (-2 * 3));
    CONSTANT_EXPR(1 * (-2UL * 3));
    CONSTANT_EXPR(1 * (-2 * 3UL));
    CONSTANT_EXPR(1LL * (-2 * 3));
    CONSTANT_EXPR(1 * (-2LL * 3));
    CONSTANT_EXPR(1 * (-2 * 3LL));
    CONSTANT_EXPR(1ULL * (-2 * 3));
    CONSTANT_EXPR(1 * (-2ULL * 3));
    CONSTANT_EXPR(1 * (-2 * 3ULL));
    CONSTANT_EXPR(1L * (-2UL * 3U));
    CONSTANT_EXPR(1U * (-2L * 3UL));
    CONSTANT_EXPR(1UL * (-2U * 3L));

    // Floats and integers mixed
    CONSTANT_EXPR(1 * 2.2);
    CONSTANT_EXPR(1.1 * 2);
    CONSTANT_EXPR(1U * 2.2);
    CONSTANT_EXPR(1.1 * 2U);
    CONSTANT_EXPR(1L * 2.2);
    CONSTANT_EXPR(1.1 * 2L);
    CONSTANT_EXPR(1UL * 2.2);
    CONSTANT_EXPR(1.1 * 2UL);

    CONSTANT_EXPR(1 * 2.2f);
    CONSTANT_EXPR(1.1f * 2);
    CONSTANT_EXPR(1U * 2.2f);
    CONSTANT_EXPR(1.1f * 2U);
    CONSTANT_EXPR(1L * 2.2f);
    CONSTANT_EXPR(1.1f * 2L);
    CONSTANT_EXPR(1UL * 2.2f);
    CONSTANT_EXPR(1.1f * 2UL);

    CONSTANT_EXPR(1 * ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) * 2);
    CONSTANT_EXPR(1U * ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) * 2U);
    CONSTANT_EXPR(1L * ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) * 2L);
    CONSTANT_EXPR(1UL * ((double) 2.2));
    CONSTANT_EXPR(((double) 1.1) * 2UL);

    CONSTANT_EXPR(-1.0 * 2.0 / 3.0);
    CONSTANT_EXPR(5.0 / 2.0 * 3.0);
    CONSTANT_EXPR((5.0 / 2.0) * 3.0);
    CONSTANT_EXPR(5.0 / (2.0 * 3.0));
    CONSTANT_EXPR(1.0 * (-2.0 * 3.0));
    CONSTANT_EXPR((4.0 * -(5.0 / 6.0)) / 1.0 * -(-(2.0 * 3.0)));

    // Floats mixed
    CONSTANT_EXPR(1.1 * 2.2);
    CONSTANT_EXPR(1.1 * 2.2f);
    CONSTANT_EXPR(1.1 * ((double) 2.2));

    CONSTANT_EXPR(1.1f * 2.2);
    CONSTANT_EXPR(1.1f * 2.2f);
    CONSTANT_EXPR(1.1f * ((double) 2.2));

    CONSTANT_EXPR(((double) 1.1) * 2.2);
    CONSTANT_EXPR(((double) 1.1) * 2.2f);
    CONSTANT_EXPR(((double) 1.1) * ((double) 2.2));

    // All in the mix
    CONSTANT_EXPR(1 * -(-(2l * 3u) / 4ul * -(5ll / 6ull) * 7.654f * -(8.76) / ((double) 9.876)));
}

TEST_FUNCTION(logical)
{
    TEST_DATA_COLUMNS;

    CONSTANT_EXPR(0 && 0);
    CONSTANT_EXPR(1 && 0);
    CONSTANT_EXPR(-1 && 0);
    CONSTANT_EXPR(0 && 1);
    CONSTANT_EXPR(0 && -1);
    CONSTANT_EXPR(1 && 1);
    CONSTANT_EXPR(1 && -1);
    CONSTANT_EXPR(-1 && -1);

    CONSTANT_EXPR(0u && 0);
    CONSTANT_EXPR(1l && 0);
    CONSTANT_EXPR(0ul && 1);
    CONSTANT_EXPR(1ll && 1);
    CONSTANT_EXPR(1ull && 1);

    CONSTANT_EXPR(0 && 0u);
    CONSTANT_EXPR(1 && 0l);
    CONSTANT_EXPR(0 && 1ul);
    CONSTANT_EXPR(1 && 1ll);
    CONSTANT_EXPR(1 && 1ull);

    CONSTANT_EXPR(0u && 0u);
    CONSTANT_EXPR(1l && 0l);
    CONSTANT_EXPR(0ul && 1ul);
    CONSTANT_EXPR(1ll && 1ll);
    CONSTANT_EXPR(1ull && 1ull);

    CONSTANT_EXPR(0 && 0.0);
    CONSTANT_EXPR(1 && 0.0);
    CONSTANT_EXPR(0 && 1.0);
    CONSTANT_EXPR(1 && 1.0);

    CONSTANT_EXPR(0.0 && 0);
    CONSTANT_EXPR(1.0 && 0);
    CONSTANT_EXPR(0.0 && 1);
    CONSTANT_EXPR(1.0 && 1);

    CONSTANT_EXPR(0.0 && 0.0);
    CONSTANT_EXPR(1.0 && 0.0);
    CONSTANT_EXPR(0.0 && 1.0);
    CONSTANT_EXPR(1.0 && 1.0);

    CONSTANT_EXPR(0.0f && 0.0f);
    CONSTANT_EXPR(1.0f && 0.0f);
    CONSTANT_EXPR(0.0f && 1.0f);
    CONSTANT_EXPR(1.0f && 1.0f);

    CONSTANT_EXPR(((double) 0.0) && ((double) 0.0));
    CONSTANT_EXPR(((double) 1.0) && ((double) 0.0));
    CONSTANT_EXPR(((double) 0.0) && ((double) 1.0));
    CONSTANT_EXPR(((double) 1.0) && ((double) 1.0));

    CONSTANT_EXPR((1 && 1) && 1);
    CONSTANT_EXPR((1 && 1) && 0);
    CONSTANT_EXPR((1 && 0) && 1);
    CONSTANT_EXPR((0 && 1) && 1);
    CONSTANT_EXPR(1 && (1 && 1));
    CONSTANT_EXPR(1 && (1 && 0));
    CONSTANT_EXPR(1 && (0 && 1));
    CONSTANT_EXPR(0 && (1 && 1));

    CONSTANT_EXPR('\1' && 1 && 1u && 1l && 1ul && 1ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\0' && 1 && 1u && 1l && 1ul && 1ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 0 && 1u && 1l && 1ul && 1ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 0u && 1l && 1ul && 1ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 1u && 0l && 1ul && 1ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 1u && 1l && 0ul && 1ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 1u && 1l && 1ul && 0ll && 1ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 1u && 1l && 1ul && 1ll && 0ull && 1.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 1u && 1l && 1ul && 1ll && 1ull && 0.0f && ((double) 1.0));
    CONSTANT_EXPR('\1' && 1 && 1u && 1l && 1ul && 1ll && 1ull && 1.0f && ((double) 0.0));


    CONSTANT_EXPR(0 || 0);
    CONSTANT_EXPR(1 || 0);
    CONSTANT_EXPR(-1 || 0);
    CONSTANT_EXPR(0 || 1);
    CONSTANT_EXPR(0 || -1);
    CONSTANT_EXPR(1 || 1);
    CONSTANT_EXPR(1 || -1);
    CONSTANT_EXPR(-1 || -1);

    CONSTANT_EXPR(0u || 0);
    CONSTANT_EXPR(1l || 0);
    CONSTANT_EXPR(0ul || 1);
    CONSTANT_EXPR(1ll || 1);
    CONSTANT_EXPR(1ull || 1);

    CONSTANT_EXPR(0 || 0u);
    CONSTANT_EXPR(1 || 0l);
    CONSTANT_EXPR(0 || 1ul);
    CONSTANT_EXPR(1 || 1ll);
    CONSTANT_EXPR(1 || 1ull);

    CONSTANT_EXPR(0u || 0u);
    CONSTANT_EXPR(1l || 0l);
    CONSTANT_EXPR(0ul || 1ul);
    CONSTANT_EXPR(1ll || 1ll);
    CONSTANT_EXPR(1ull || 1ull);

    CONSTANT_EXPR(0 || 0.0);
    CONSTANT_EXPR(1 || 0.0);
    CONSTANT_EXPR(0 || 1.0);
    CONSTANT_EXPR(1 || 1.0);

    CONSTANT_EXPR(0.0 || 0);
    CONSTANT_EXPR(1.0 || 0);
    CONSTANT_EXPR(0.0 || 1);
    CONSTANT_EXPR(1.0 || 1);

    CONSTANT_EXPR(0.0 || 0.0);
    CONSTANT_EXPR(1.0 || 0.0);
    CONSTANT_EXPR(0.0 || 1.0);
    CONSTANT_EXPR(1.0 || 1.0);

    CONSTANT_EXPR(0.0f || 0.0f);
    CONSTANT_EXPR(1.0f || 0.0f);
    CONSTANT_EXPR(0.0f || 1.0f);
    CONSTANT_EXPR(1.0f || 1.0f);

    CONSTANT_EXPR(((double) 0.0) || ((double) 0.0));
    CONSTANT_EXPR(((double) 1.0) || ((double) 0.0));
    CONSTANT_EXPR(((double) 0.0) || ((double) 1.0));
    CONSTANT_EXPR(((double) 1.0) || ((double) 1.0));

    CONSTANT_EXPR((0 || 0) || 0);
    CONSTANT_EXPR((0 || 0) || 1);
    CONSTANT_EXPR((0 || 1) || 0);
    CONSTANT_EXPR((1 || 0) || 0);
    CONSTANT_EXPR(0 || (0 || 0));
    CONSTANT_EXPR(0 || (0 || 1));
    CONSTANT_EXPR(0 || (1 || 0));
    CONSTANT_EXPR(1 || (0 || 0));

    CONSTANT_EXPR('\0' || 0 || 0u || 0l || 0ul || 0ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\1' || 0 || 0u || 0l || 0ul || 0ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 1 || 0u || 0l || 0ul || 0ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 1u || 0l || 0ul || 0ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 0u || 1l || 0ul || 0ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 0u || 0l || 1ul || 0ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 0u || 0l || 0ul || 1ll || 0ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 0u || 0l || 0ul || 0ll || 1ull || 0.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 0u || 0l || 0ul || 0ll || 0ull || 1.0f || ((double) 0.0));
    CONSTANT_EXPR('\0' || 0 || 0u || 0l || 0ul || 0ll || 0ull || 0.0f || ((double) 1.0));
}

TEST_FUNCTION(bitwise)
{
    TEST_DATA_COLUMNS;

    CONSTANT_EXPR(0xcafe0000 | 0x0000babe);
    CONSTANT_EXPR(0xcafebabe & 0x00ffff00);
    CONSTANT_EXPR(0xcafebabe ^ 0x00ffff00);

    CONSTANT_EXPR('\1' | '\3');
    CONSTANT_EXPR('\1' | 3);
    CONSTANT_EXPR('\1' | 3u);
    CONSTANT_EXPR('\1' | 3l);
    CONSTANT_EXPR('\1' | 3ul);
    CONSTANT_EXPR('\1' | 3ll);
    CONSTANT_EXPR('\1' | 3ull);

    CONSTANT_EXPR(1 | '\3');
    CONSTANT_EXPR(1 | 3);
    CONSTANT_EXPR(1 | 3u);
    CONSTANT_EXPR(1 | 3l);
    CONSTANT_EXPR(1 | 3ul);
    CONSTANT_EXPR(1 | 3ll);
    CONSTANT_EXPR(1 | 3ull);

    CONSTANT_EXPR(1u | '\3');
    CONSTANT_EXPR(1u | 3);
    CONSTANT_EXPR(1u | 3u);
    CONSTANT_EXPR(1u | 3l);
    CONSTANT_EXPR(1u | 3ul);
    CONSTANT_EXPR(1u | 3ll);
    CONSTANT_EXPR(1u | 3ull);

    CONSTANT_EXPR(1l | '\3');
    CONSTANT_EXPR(1l | 3);
    CONSTANT_EXPR(1l | 3u);
    CONSTANT_EXPR(1l | 3l);
    CONSTANT_EXPR(1l | 3ul);
    CONSTANT_EXPR(1l | 3ll);
    CONSTANT_EXPR(1l | 3ull);

    CONSTANT_EXPR(1ul | '\3');
    CONSTANT_EXPR(1ul | 3);
    CONSTANT_EXPR(1ul | 3u);
    CONSTANT_EXPR(1ul | 3l);
    CONSTANT_EXPR(1ul | 3ul);
    CONSTANT_EXPR(1ul | 3ll);
    CONSTANT_EXPR(1ul | 3ull);

    CONSTANT_EXPR(1ll | '\3');
    CONSTANT_EXPR(1ll | 3);
    CONSTANT_EXPR(1ll | 3u);
    CONSTANT_EXPR(1ll | 3l);
    CONSTANT_EXPR(1ll | 3ul);
    CONSTANT_EXPR(1ll | 3ll);
    CONSTANT_EXPR(1ll | 3ull);

    CONSTANT_EXPR(1ull | '\3');
    CONSTANT_EXPR(1ull | 3);
    CONSTANT_EXPR(1ull | 3u);
    CONSTANT_EXPR(1ull | 3l);
    CONSTANT_EXPR(1ull | 3ul);
    CONSTANT_EXPR(1ull | 3ll);
    CONSTANT_EXPR(1ull | 3ull);

    CONSTANT_EXPR('\1' & '\3');
    CONSTANT_EXPR('\1' & 3);
    CONSTANT_EXPR('\1' & 3u);
    CONSTANT_EXPR('\1' & 3l);
    CONSTANT_EXPR('\1' & 3ul);
    CONSTANT_EXPR('\1' & 3ll);
    CONSTANT_EXPR('\1' & 3ull);

    CONSTANT_EXPR(1 & '\3');
    CONSTANT_EXPR(1 & 3);
    CONSTANT_EXPR(1 & 3u);
    CONSTANT_EXPR(1 & 3l);
    CONSTANT_EXPR(1 & 3ul);
    CONSTANT_EXPR(1 & 3ll);
    CONSTANT_EXPR(1 & 3ull);

    CONSTANT_EXPR(1u & '\3');
    CONSTANT_EXPR(1u & 3);
    CONSTANT_EXPR(1u & 3u);
    CONSTANT_EXPR(1u & 3l);
    CONSTANT_EXPR(1u & 3ul);
    CONSTANT_EXPR(1u & 3ll);
    CONSTANT_EXPR(1u & 3ull);

    CONSTANT_EXPR(1l & '\3');
    CONSTANT_EXPR(1l & 3);
    CONSTANT_EXPR(1l & 3u);
    CONSTANT_EXPR(1l & 3l);
    CONSTANT_EXPR(1l & 3ul);
    CONSTANT_EXPR(1l & 3ll);
    CONSTANT_EXPR(1l & 3ull);

    CONSTANT_EXPR(1ul & '\3');
    CONSTANT_EXPR(1ul & 3);
    CONSTANT_EXPR(1ul & 3u);
    CONSTANT_EXPR(1ul & 3l);
    CONSTANT_EXPR(1ul & 3ul);
    CONSTANT_EXPR(1ul & 3ll);
    CONSTANT_EXPR(1ul & 3ull);

    CONSTANT_EXPR(1ll & '\3');
    CONSTANT_EXPR(1ll & 3);
    CONSTANT_EXPR(1ll & 3u);
    CONSTANT_EXPR(1ll & 3l);
    CONSTANT_EXPR(1ll & 3ul);
    CONSTANT_EXPR(1ll & 3ll);
    CONSTANT_EXPR(1ll & 3ull);

    CONSTANT_EXPR(1ull & '\3');
    CONSTANT_EXPR(1ull & 3);
    CONSTANT_EXPR(1ull & 3u);
    CONSTANT_EXPR(1ull & 3l);
    CONSTANT_EXPR(1ull & 3ul);
    CONSTANT_EXPR(1ull & 3ll);
    CONSTANT_EXPR(1ull & 3ull);

    CONSTANT_EXPR('\1' ^ '\3');
    CONSTANT_EXPR('\1' ^ 3);
    CONSTANT_EXPR('\1' ^ 3u);
    CONSTANT_EXPR('\1' ^ 3l);
    CONSTANT_EXPR('\1' ^ 3ul);
    CONSTANT_EXPR('\1' ^ 3ll);
    CONSTANT_EXPR('\1' ^ 3ull);

    CONSTANT_EXPR(1 ^ '\3');
    CONSTANT_EXPR(1 ^ 3);
    CONSTANT_EXPR(1 ^ 3u);
    CONSTANT_EXPR(1 ^ 3l);
    CONSTANT_EXPR(1 ^ 3ul);
    CONSTANT_EXPR(1 ^ 3ll);
    CONSTANT_EXPR(1 ^ 3ull);

    CONSTANT_EXPR(1u ^ '\3');
    CONSTANT_EXPR(1u ^ 3);
    CONSTANT_EXPR(1u ^ 3u);
    CONSTANT_EXPR(1u ^ 3l);
    CONSTANT_EXPR(1u ^ 3ul);
    CONSTANT_EXPR(1u ^ 3ll);
    CONSTANT_EXPR(1u ^ 3ull);

    CONSTANT_EXPR(1l ^ '\3');
    CONSTANT_EXPR(1l ^ 3);
    CONSTANT_EXPR(1l ^ 3u);
    CONSTANT_EXPR(1l ^ 3l);
    CONSTANT_EXPR(1l ^ 3ul);
    CONSTANT_EXPR(1l ^ 3ll);
    CONSTANT_EXPR(1l ^ 3ull);

    CONSTANT_EXPR(1ul ^ '\3');
    CONSTANT_EXPR(1ul ^ 3);
    CONSTANT_EXPR(1ul ^ 3u);
    CONSTANT_EXPR(1ul ^ 3l);
    CONSTANT_EXPR(1ul ^ 3ul);
    CONSTANT_EXPR(1ul ^ 3ll);
    CONSTANT_EXPR(1ul ^ 3ull);

    CONSTANT_EXPR(1ll ^ '\3');
    CONSTANT_EXPR(1ll ^ 3);
    CONSTANT_EXPR(1ll ^ 3u);
    CONSTANT_EXPR(1ll ^ 3l);
    CONSTANT_EXPR(1ll ^ 3ul);
    CONSTANT_EXPR(1ll ^ 3ll);
    CONSTANT_EXPR(1ll ^ 3ull);

    CONSTANT_EXPR(1ull ^ '\3');
    CONSTANT_EXPR(1ull ^ 3);
    CONSTANT_EXPR(1ull ^ 3u);
    CONSTANT_EXPR(1ull ^ 3l);
    CONSTANT_EXPR(1ull ^ 3ul);
    CONSTANT_EXPR(1ull ^ 3ll);
    CONSTANT_EXPR(1ull ^ 3ull);
}


TEST_FUNCTION(equality)
{
    TEST_DATA_COLUMNS;

    CONSTANT_EXPR(1 == '\2');
    CONSTANT_EXPR(1 == 2);
    CONSTANT_EXPR(1 == 2u);
    CONSTANT_EXPR(1 == 2l);
    CONSTANT_EXPR(1 == 2ul);
    CONSTANT_EXPR(1 == 2ll);
    CONSTANT_EXPR(1 == 2ull);
    CONSTANT_EXPR(1 == 2.0);
    CONSTANT_EXPR(1 == 2.0f);
    CONSTANT_EXPR(1 == ((double) 2.0));

    CONSTANT_EXPR(1 != '\2');
    CONSTANT_EXPR(1 != 2);
    CONSTANT_EXPR(1 != 2u);
    CONSTANT_EXPR(1 != 2l);
    CONSTANT_EXPR(1 != 2ul);
    CONSTANT_EXPR(1 != 2ll);
    CONSTANT_EXPR(1 != 2ull);
    CONSTANT_EXPR(1 != 2.0);
    CONSTANT_EXPR(1 != 2.0f);
    CONSTANT_EXPR(1 != ((double) 2.0));

    CONSTANT_EXPR(-1 == '\2');
    CONSTANT_EXPR(-1 == 2);
    CONSTANT_EXPR(-1 == 2u);
    CONSTANT_EXPR(-1 == 2l);
    CONSTANT_EXPR(-1 == 2ul);
    CONSTANT_EXPR(-1 == 2ll);
    CONSTANT_EXPR(-1 == 2ull);
    CONSTANT_EXPR(-1 == 2.0);
    CONSTANT_EXPR(-1 == 2.0f);
    CONSTANT_EXPR(-1 == ((double) 2.0));

    CONSTANT_EXPR(-1 != '\2');
    CONSTANT_EXPR(-1 != 2);
    CONSTANT_EXPR(-1 != 2u);
    CONSTANT_EXPR(-1 != 2l);
    CONSTANT_EXPR(-1 != 2ul);
    CONSTANT_EXPR(-1 != 2ll);
    CONSTANT_EXPR(-1 != 2ull);
    CONSTANT_EXPR(-1 != 2.0);
    CONSTANT_EXPR(-1 != 2.0f);
    CONSTANT_EXPR(-1 != ((double) 2.0));
}


TEST_FUNCTION(relational)
{
    TEST_DATA_COLUMNS;

    CONSTANT_EXPR(1 >= '\2');
    CONSTANT_EXPR(1 >= 2);
    CONSTANT_EXPR(1 >= 2u);
    CONSTANT_EXPR(1 >= 2l);
    CONSTANT_EXPR(1 >= 2ul);
    CONSTANT_EXPR(1 >= 2ll);
    CONSTANT_EXPR(1 >= 2ull);
    CONSTANT_EXPR(1 >= 2.0);
    CONSTANT_EXPR(1 >= 2.0f);
    CONSTANT_EXPR(1 >= ((double) 2.0));

    CONSTANT_EXPR(1 > '\2');
    CONSTANT_EXPR(1 > 2);
    CONSTANT_EXPR(1 > 2u);
    CONSTANT_EXPR(1 > 2l);
    CONSTANT_EXPR(1 > 2ul);
    CONSTANT_EXPR(1 > 2ll);
    CONSTANT_EXPR(1 > 2ull);
    CONSTANT_EXPR(1 > 2.0);
    CONSTANT_EXPR(1 > 2.0f);
    CONSTANT_EXPR(1 > ((double) 2.0));

    CONSTANT_EXPR(1 <= '\2');
    CONSTANT_EXPR(1 <= 2);
    CONSTANT_EXPR(1 <= 2u);
    CONSTANT_EXPR(1 <= 2l);
    CONSTANT_EXPR(1 <= 2ul);
    CONSTANT_EXPR(1 <= 2ll);
    CONSTANT_EXPR(1 <= 2ull);
    CONSTANT_EXPR(1 <= 2.0);
    CONSTANT_EXPR(1 <= 2.0f);
    CONSTANT_EXPR(1 <= ((double) 2.0));

    CONSTANT_EXPR(1 < '\2');
    CONSTANT_EXPR(1 < 2);
    CONSTANT_EXPR(1 < 2u);
    CONSTANT_EXPR(1 < 2l);
    CONSTANT_EXPR(1 < 2ul);
    CONSTANT_EXPR(1 < 2ll);
    CONSTANT_EXPR(1 < 2ull);
    CONSTANT_EXPR(1 < 2.0);
    CONSTANT_EXPR(1 < 2.0f);
    CONSTANT_EXPR(1 < ((double) 2.0));

    CONSTANT_EXPR(-1 >= '\2');
    CONSTANT_EXPR(-1 >= 2);
    CONSTANT_EXPR(-1 >= 2u);
    CONSTANT_EXPR(-1 >= 2l);
    CONSTANT_EXPR(-1 >= 2ul);
    CONSTANT_EXPR(-1 >= 2ll);
    CONSTANT_EXPR(-1 >= 2ull);
    CONSTANT_EXPR(-1 >= 2.0);
    CONSTANT_EXPR(-1 >= 2.0f);
    CONSTANT_EXPR(-1 >= ((double) 2.0));

    CONSTANT_EXPR(-1 > '\2');
    CONSTANT_EXPR(-1 > 2);
    CONSTANT_EXPR(-1 > 2u);
    CONSTANT_EXPR(-1 > 2l);
    CONSTANT_EXPR(-1 > 2ul);
    CONSTANT_EXPR(-1 > 2ll);
    CONSTANT_EXPR(-1 > 2ull);
    CONSTANT_EXPR(-1 > 2.0);
    CONSTANT_EXPR(-1 > 2.0f);
    CONSTANT_EXPR(-1 > ((double) 2.0));

    CONSTANT_EXPR(-1 <= '\2');
    CONSTANT_EXPR(-1 <= 2);
    CONSTANT_EXPR(-1 <= 2u);
    CONSTANT_EXPR(-1 <= 2l);
    CONSTANT_EXPR(-1 <= 2ul);
    CONSTANT_EXPR(-1 <= 2ll);
    CONSTANT_EXPR(-1 <= 2ull);
    CONSTANT_EXPR(-1 <= 2.0);
    CONSTANT_EXPR(-1 <= 2.0f);
    CONSTANT_EXPR(-1 <= ((double) 2.0));

    CONSTANT_EXPR(-1 < '\2');
    CONSTANT_EXPR(-1 < 2);
    CONSTANT_EXPR(-1 < 2u);
    CONSTANT_EXPR(-1 < 2l);
    CONSTANT_EXPR(-1 < 2ul);
    CONSTANT_EXPR(-1 < 2ll);
    CONSTANT_EXPR(-1 < 2ull);
    CONSTANT_EXPR(-1 < 2.0);
    CONSTANT_EXPR(-1 < 2.0f);
    CONSTANT_EXPR(-1 < ((double) 2.0));
}


TEST_FUNCTION(shift)
{
    TEST_DATA_COLUMNS;

    CONSTANT_EXPR(1 << '\2');
    CONSTANT_EXPR(1 << 2);
    CONSTANT_EXPR(1 << 2u);
    CONSTANT_EXPR(1 << 2l);
    CONSTANT_EXPR(1 << 2ul);
    CONSTANT_EXPR(1 << 2ll);
    CONSTANT_EXPR(1 << 2ull);

    CONSTANT_EXPR(1u << '\2');
    CONSTANT_EXPR(1u << 2);
    CONSTANT_EXPR(1u << 2u);
    CONSTANT_EXPR(1u << 2l);
    CONSTANT_EXPR(1u << 2ul);
    CONSTANT_EXPR(1u << 2ll);
    CONSTANT_EXPR(1u << 2ull);

    CONSTANT_EXPR(1ll << '\2');
    CONSTANT_EXPR(1ll << 2);
    CONSTANT_EXPR(1ll << 2u);
    CONSTANT_EXPR(1ll << 2l);
    CONSTANT_EXPR(1ll << 2ul);
    CONSTANT_EXPR(1ll << 2ll);
    CONSTANT_EXPR(1ll << 2ull);

    CONSTANT_EXPR(1ull << '\2');
    CONSTANT_EXPR(1ull << 2);
    CONSTANT_EXPR(1ull << 2u);
    CONSTANT_EXPR(1ull << 2l);
    CONSTANT_EXPR(1ull << 2ul);
    CONSTANT_EXPR(1ull << 2ll);
    CONSTANT_EXPR(1ull << 2ull);

    CONSTANT_EXPR(-1 >> '\2');
    CONSTANT_EXPR(-1 >> 2);
    CONSTANT_EXPR(-1 >> 2u);
    CONSTANT_EXPR(-1 >> 2l);
    CONSTANT_EXPR(-1 >> 2ul);
    CONSTANT_EXPR(-1 >> 2ll);
    CONSTANT_EXPR(-1 >> 2ull);

    CONSTANT_EXPR(0xffffffffu >> '\2');
    CONSTANT_EXPR(0xffffffffu >> 2);
    CONSTANT_EXPR(0xffffffffu >> 2u);
    CONSTANT_EXPR(0xffffffffu >> 2l);
    CONSTANT_EXPR(0xffffffffu >> 2ul);
    CONSTANT_EXPR(0xffffffffu >> 2ll);
    CONSTANT_EXPR(0xffffffffu >> 2ull);

    CONSTANT_EXPR(-1ll >> '\2');
    CONSTANT_EXPR(-1ll >> 2);
    CONSTANT_EXPR(-1ll >> 2u);
    CONSTANT_EXPR(-1ll >> 2l);
    CONSTANT_EXPR(-1ll >> 2ul);
    CONSTANT_EXPR(-1ll >> 2ll);
    CONSTANT_EXPR(-1ll >> 2ull);

    CONSTANT_EXPR(0xffffffffffffffffull >> '\2');
    CONSTANT_EXPR(0xffffffffffffffffull >> 2);
    CONSTANT_EXPR(0xffffffffffffffffull >> 2u);
    CONSTANT_EXPR(0xffffffffffffffffull >> 2l);
    CONSTANT_EXPR(0xffffffffffffffffull >> 2ul);
    CONSTANT_EXPR(0xffffffffffffffffull >> 2ll);
    CONSTANT_EXPR(0xffffffffffffffffull >> 2ull);
}


TEST_FUNCTION(unary)
{
    TEST_DATA_COLUMNS;

    CONSTANT_EXPR(~'\0');
    CONSTANT_EXPR(~0);
    CONSTANT_EXPR(~0u);
    CONSTANT_EXPR(~0l);
    CONSTANT_EXPR(~0ul);
    CONSTANT_EXPR(~0ll);
    CONSTANT_EXPR(~0ull);

    CONSTANT_EXPR(~'\1');
    CONSTANT_EXPR(~1);
    CONSTANT_EXPR(~1u);
    CONSTANT_EXPR(~1l);
    CONSTANT_EXPR(~1ul);
    CONSTANT_EXPR(~1ll);
    CONSTANT_EXPR(~1ull);

    CONSTANT_EXPR(~'\xf0');
    CONSTANT_EXPR(~0xf0f0f0f0);
    CONSTANT_EXPR(~0xf0f0f0f0u);
    CONSTANT_EXPR(~0xf0f0f0f0l);
    CONSTANT_EXPR(~0xf0f0f0f0ul);
    CONSTANT_EXPR(~0x0f0f0f0f0f0f0f0fll);
    CONSTANT_EXPR(~0x0f0f0f0f0f0f0f0full);
    CONSTANT_EXPR(~0xf0f0f0f0f0f0f0f0ll);
    CONSTANT_EXPR(~0xf0f0f0f0f0f0f0f0ull);

    CONSTANT_EXPR(~'\xff');
    CONSTANT_EXPR(~0xffffffff);
    CONSTANT_EXPR(~0xffffffffu);
    CONSTANT_EXPR(~0xffffffffl);
    CONSTANT_EXPR(~0xfffffffful);
    CONSTANT_EXPR(~0x7fffffffffffffffll);
    CONSTANT_EXPR(~0x8000000000000000ll);
    CONSTANT_EXPR(~0xffffffffffffffffll);
    CONSTANT_EXPR(~0xffffffffffffffffull);

    CONSTANT_EXPR(!0);
    CONSTANT_EXPR(!1);
    CONSTANT_EXPR(!-1);

    CONSTANT_EXPR(!0u);
    CONSTANT_EXPR(!1u);
    CONSTANT_EXPR(!-1u);

    CONSTANT_EXPR(!0l);
    CONSTANT_EXPR(!1l);
    CONSTANT_EXPR(!-1l);

    CONSTANT_EXPR(!0ul);
    CONSTANT_EXPR(!1ul);
    CONSTANT_EXPR(!-1ul);

    CONSTANT_EXPR(!0ll);
    CONSTANT_EXPR(!1ll);
    CONSTANT_EXPR(!-1ll);

    CONSTANT_EXPR(!0ull);
    CONSTANT_EXPR(!1ull);
    CONSTANT_EXPR(!-1ull);

    CONSTANT_EXPR(!0.0);
    CONSTANT_EXPR(!1.0);
    CONSTANT_EXPR(!-1.0);

    CONSTANT_EXPR(!0.0f);
    CONSTANT_EXPR(!1.0f);
    CONSTANT_EXPR(!-1.0f);

    CONSTANT_EXPR(!((double) 0.0));
    CONSTANT_EXPR(!((double) 1.0));
    CONSTANT_EXPR(!((double) -1.0));
}


TEST_FUNCTION(builtins)
{
    TEST_DATA_COLUMNS;

    struct A {
           long l;
           short s;
           int i;
           char c;
    };

    struct B {
           int j;
           struct A a[4];
           struct A* pa;
           struct B* pb;
    };

    struct A a;
    struct B b;

    // alignof
    CONSTANT_EXPR(__alignof__(char));
    CONSTANT_EXPR(__alignof__(short));
    CONSTANT_EXPR(__alignof__(int));
    CONSTANT_EXPR(__alignof__(long));

    CONSTANT_EXPR(__alignof__(unsigned char));
    CONSTANT_EXPR(__alignof__(unsigned short));
    CONSTANT_EXPR(__alignof__(unsigned int));
    CONSTANT_EXPR(__alignof__(unsigned long));

    CONSTANT_EXPR(__alignof__(float));
    CONSTANT_EXPR(__alignof__(double));

    CONSTANT_EXPR(__alignof__(struct A));
    CONSTANT_EXPR(__alignof__(struct A*));
    CONSTANT_EXPR(__alignof__(a));
    CONSTANT_EXPR(__alignof__(typeof(a)));
    CONSTANT_EXPR(__alignof__(&a));
    CONSTANT_EXPR(__alignof__(typeof(&a)));
    CONSTANT_EXPR(__alignof__(a.l));
    CONSTANT_EXPR(__alignof__(a.s));
    CONSTANT_EXPR(__alignof__(a.i));
    CONSTANT_EXPR(__alignof__(a.c));

    CONSTANT_EXPR(__alignof__(struct B));
    CONSTANT_EXPR(__alignof__(b));
    CONSTANT_EXPR(__alignof__(b.j));
    CONSTANT_EXPR(__alignof__(b.a));
    CONSTANT_EXPR(__alignof__(b.a[1]));
    CONSTANT_EXPR(__alignof__(b.a[2].s));
    CONSTANT_EXPR(__alignof__(b.pa));
    CONSTANT_EXPR(__alignof__(b.pb->j));
    CONSTANT_EXPR(__alignof__(b.pb->a));
    CONSTANT_EXPR(__alignof__(b.pb->a[1]));
    CONSTANT_EXPR(__alignof__(b.pb->a[2].s));
    CONSTANT_EXPR(__alignof__("foo"));

    // __builtin_choose_expr, only available for C
    CONSTANT_EXPR2(__builtin_choose_expr(0, '\2', '\3'), '\3');
    CONSTANT_EXPR2(__builtin_choose_expr(1, '\2', '\3'), '\2');
    CONSTANT_EXPR2(__builtin_choose_expr(0, 2, 3), 3);
    CONSTANT_EXPR2(__builtin_choose_expr(1, 2, 3), 2);
    CONSTANT_EXPR2(__builtin_choose_expr(0, 2u, 3u), 3u);
    CONSTANT_EXPR2(__builtin_choose_expr(1, 2u, 3u), 2u);
    CONSTANT_EXPR2(__builtin_choose_expr(0, 2l, 3l), 3l);
    CONSTANT_EXPR2(__builtin_choose_expr(1, 2l, 3l), 2l);
    CONSTANT_EXPR2(__builtin_choose_expr(0, 2lu, 3lu), 3lu);
    CONSTANT_EXPR2(__builtin_choose_expr(1, 2lu, 3lu), 2lu);
    CONSTANT_EXPR2(__builtin_choose_expr(0, 2ll, 3ll), 3ll);
    CONSTANT_EXPR2(__builtin_choose_expr(1, 2ll, 3ll), 2ll);
    CONSTANT_EXPR2(__builtin_choose_expr(0, 2ull, 3ull), 3ull);
    CONSTANT_EXPR2(__builtin_choose_expr(1, 2ull, 3ull), 2ull);

    // __builtin_constant_p
    CONSTANT_EXPR(__builtin_constant_p(0));
    CONSTANT_EXPR(__builtin_constant_p(1));
    CONSTANT_EXPR(__builtin_constant_p(-1));
    CONSTANT_EXPR(__builtin_constant_p('\1'));
    CONSTANT_EXPR(__builtin_constant_p(1u));
    CONSTANT_EXPR(__builtin_constant_p(1l));
    CONSTANT_EXPR(__builtin_constant_p(1ul));
    CONSTANT_EXPR(__builtin_constant_p(1ll));
    CONSTANT_EXPR(__builtin_constant_p(1ull));
    CONSTANT_EXPR(__builtin_constant_p(1.0));
    CONSTANT_EXPR(__builtin_constant_p(1.0f));
    CONSTANT_EXPR(__builtin_constant_p(((double) 1.0)));
//    CONSTANT_EXPR(__builtin_constant_p("foo"));
    CONSTANT_EXPR(__builtin_constant_p(a));
    CONSTANT_EXPR(__builtin_constant_p(b));

    //__builtin_expect
    CONSTANT_EXPR_FAIL(__builtin_expect(0, 0), efSize);
    CONSTANT_EXPR_FAIL(__builtin_expect(1, 0), efSize);
    CONSTANT_EXPR_FAIL(__builtin_expect(-1, 0), efSize);

    // offsetof
    CONSTANT_EXPR(__builtin_offsetof(struct A, l));
    CONSTANT_EXPR(__builtin_offsetof(struct A, s));
    CONSTANT_EXPR(__builtin_offsetof(struct A, i));
    CONSTANT_EXPR(__builtin_offsetof(struct A, c));
    CONSTANT_EXPR(__builtin_offsetof(struct B, j));
    CONSTANT_EXPR(__builtin_offsetof(struct B, a));
    CONSTANT_EXPR(__builtin_offsetof(struct B, a[1]));
    CONSTANT_EXPR(__builtin_offsetof(struct B, a[2].s));
    CONSTANT_EXPR(__builtin_offsetof(struct B, pa));
    CONSTANT_EXPR(__builtin_offsetof(struct B, pb));
//    CONSTANT_EXPR(__builtin_offsetof(typeof(a), c));
//    CONSTANT_EXPR(__builtin_offsetof(typeof(b), a[2].s));

//    __builtin_object_size
    CONSTANT_EXPR(__builtin_object_size(b.pa, 0));
    CONSTANT_EXPR(__builtin_object_size(b.pa, 1));
    CONSTANT_EXPR(__builtin_object_size(b.pa, 2));
    CONSTANT_EXPR(__builtin_object_size(b.pa, 3));

    // __builtin_types_compatible_p, only available for C
    CONSTANT_EXPR2(__builtin_types_compatible_p(char, int), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(int, int), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(const int, int), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(volatile int, int), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(unsigned int, int), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(long, long long),
                   (_specs->sizeofLong == 4) ? 0 : 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(long, char*), 0);

    CONSTANT_EXPR2(__builtin_types_compatible_p(float, int), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(double, int), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(double, float), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(double, double), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(double, typeof(1.0)), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(double, typeof(1.0f)), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(double, typeof(((double) 1.0))), 1);

    CONSTANT_EXPR2(__builtin_types_compatible_p(short*, short*), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(short*, short**), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(short**, short**), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(short*[], short*[]), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(int[], int[4]), 1);

    CONSTANT_EXPR2(__builtin_types_compatible_p(struct A, struct B), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(struct A, typeof(a)), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(struct A*, typeof(&a)), 1);
    CONSTANT_EXPR2(__builtin_types_compatible_p(struct A, typeof(b)), 0);
    CONSTANT_EXPR2(__builtin_types_compatible_p(struct B*, typeof(&b)), 1);

    // sizeof
    CONSTANT_EXPR(sizeof(char));
    CONSTANT_EXPR(sizeof(short));
    CONSTANT_EXPR(sizeof(int));
    CONSTANT_EXPR(sizeof(long));

    CONSTANT_EXPR(sizeof(unsigned char));
    CONSTANT_EXPR(sizeof(unsigned short));
    CONSTANT_EXPR(sizeof(unsigned int));
    CONSTANT_EXPR(sizeof(unsigned long));

    CONSTANT_EXPR(sizeof(float));
    CONSTANT_EXPR(sizeof(double));

    CONSTANT_EXPR(sizeof(struct A));
    CONSTANT_EXPR(sizeof(struct A*));
    CONSTANT_EXPR(sizeof(a));
    CONSTANT_EXPR(sizeof(typeof(a)));
    CONSTANT_EXPR(sizeof(&a));
    CONSTANT_EXPR(sizeof(typeof(&a)));
    CONSTANT_EXPR(sizeof(a.l));
    CONSTANT_EXPR(sizeof(a.s));
    CONSTANT_EXPR(sizeof(a.i));
    CONSTANT_EXPR(sizeof(a.c));

    CONSTANT_EXPR(sizeof(struct B));
    CONSTANT_EXPR(sizeof(b));
    CONSTANT_EXPR(sizeof(b.j));
    CONSTANT_EXPR(sizeof(b.a));
    CONSTANT_EXPR(sizeof(b.a[0]));
    CONSTANT_EXPR(sizeof(b.a[0].s));
    CONSTANT_EXPR(sizeof(b.pa));
    CONSTANT_EXPR(sizeof(b.pb->j));
    CONSTANT_EXPR(sizeof(b.pb->a));
    CONSTANT_EXPR(sizeof(b.pb->a[0]));
    CONSTANT_EXPR(sizeof(b.pb->a[0].s));
//    CONSTANT_EXPR_FAIL(sizeof(typeof(main)), efResult);

    CONSTANT_EXPR(sizeof("foo"));
    CONSTANT_EXPR(sizeof("foo" "bar"));
    CONSTANT_EXPR(sizeof(typeof("foo" "bar")));
    CONSTANT_EXPR(sizeof("\n"));
    CONSTANT_EXPR(sizeof("\t"));
    CONSTANT_EXPR(sizeof("\\\"\\\""));
    CONSTANT_EXPR(sizeof("\0"));
    CONSTANT_EXPR(sizeof("\377"));
    CONSTANT_EXPR(sizeof("\377777"));
    CONSTANT_EXPR(sizeof("\777777"));
    CONSTANT_EXPR(sizeof("\x0"));
    CONSTANT_EXPR(sizeof("\xff"));
    CONSTANT_EXPR(sizeof("\x0foo"));
    CONSTANT_EXPR(sizeof("\xffffffffffffffff"));
    CONSTANT_EXPR(sizeof("foo\x0"));
    CONSTANT_EXPR(sizeof("foo\xff"));
}


template <class T>
ExpressionResultSize ASTExpressionEvaluatorTester::exprSize(T x) const
{
    // Float or integer type?
    x = 1.5;
    if (x > 1) {
        return (sizeof(T) == 4) ? esFloat : esDouble;
    }
    else {
        // Signed or unsigned type?
        x = -1;
        switch (sizeof(T)) {
        case 1:  return (x > 0) ? esUInt8  : esInt8;
        case 2:  return (x > 0) ? esUInt16 : esInt16;
        case 4:  return (x > 0) ? esUInt32 : esInt32;
        default: return (x > 0) ? esUInt64 : esInt64;
        }
    }
}

