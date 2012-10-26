#include <QString>
#include <QtTest>
#include <memspecs.h>
#include <symfactory.h>
#include <kernelsymbolparser.h>
#include <variable.h>
#include <basetype.h>
#include <typefilter.h>
#include <shell.h>

#define safe_delete(x) \
    do { if ((x)) { delete (x); (x) = 0; } } while (0)

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


class TypeFilterTest : public QObject
{
    Q_OBJECT
    
public:
    TypeFilterTest();
    
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void parseDataType();
    void parseTypeName();
    void parseVarName();
    void parseFileName();
    void parseSize();

private:
    SymFactory* _factory;
    MemSpecs* _specs;
    const Variable* var_a;
    const Variable* var_b;
    const BaseType* type_a;
    const BaseType* type_b;
    const BaseType* type_int;
};


TypeFilterTest::TypeFilterTest()
    : _factory(0), _specs(0)
{
    shell = new Shell(false);
}


void TypeFilterTest::initTestCase()
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

    var_a = _factory->findVarByName("a");
    var_b = _factory->findVarByName("b");
    type_a = var_a->refType();
    type_b = var_b->refType();
    type_int = _factory->findBaseTypeByName("int");
}


void TypeFilterTest::cleanupTestCase()
{
    safe_delete(_factory);
    safe_delete(_specs);
}


void TypeFilterTest::parseDataType()
{
    TypeFilter f;

    f.parseOption("datatype", "struct");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("datatype", "struct,union");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("datatype", "*int*,struct,union");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("datatype", "str*");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("datatype", "*int*");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("datatype", "/int/");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("datatype", "/U?[Ii]nt/");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("datatype", "/Int|Struct/");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("datatype", "/int/i");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    // Whitespace
    f.parseOption("datatype", "  struct");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("datatype", "struct  ");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("datatype", "  struct  ");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    // Case sensitivity
    f.parseOption("datatype", "StRuCt");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));
}


void TypeFilterTest::parseTypeName()
{
    TypeFilter f;

    f.parseOption("typename", "A");
    QVERIFY(f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "a");
    QVERIFY(f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "B");
    QVERIFY(!f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "b");
    QVERIFY(!f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "int");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("typename", "  int");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("typename", "int  ");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("typename", "  int  ");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("typename", "InT");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("typename", "*a*");
    QVERIFY(f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "*i*");
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));

    f.parseOption("typename", "/[AB]/");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "/[Ab]/");
    QVERIFY(f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(!f.match(type_int));

    f.parseOption("typename", "/[Ab]/i");
    QVERIFY(f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));
}


void TypeFilterTest::parseVarName()
{
    VariableFilter f;

    f.parseOption("variablename", "A");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "a");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "B");
    QVERIFY(!f.match(var_a));
    QVERIFY(f.match(var_b));

    f.parseOption("variablename", "b");
    QVERIFY(!f.match(var_a));
    QVERIFY(f.match(var_b));

    f.parseOption("variablename", "/A/");
    QVERIFY(!f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "/A/i");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "/a/");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "/^a$/");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "*A*");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "*a*");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "  A");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "A  ");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "  A  ");
    QVERIFY(f.match(var_a));
    QVERIFY(!f.match(var_b));

    f.parseOption("variablename", "/[ab]/");
    QVERIFY(f.match(var_a));
    QVERIFY(f.match(var_b));

    f.parseOption("variablename", "foo");
    QVERIFY(!f.match(var_a));
    QVERIFY(!f.match(var_b));
}


void TypeFilterTest::parseFileName()
{
    VariableFilter f;

    // Cannot be tested, requires further changes in KernelSymbolParser
    f.parseOption("filename", "vmlinux");
    QVERIFY(!f.match(var_a));
    QVERIFY(!f.match(var_b));
}


void TypeFilterTest::parseSize()
{
    TypeFilter f;
    VariableFilter vf;

    f.parseOption("size", QString::number(type_a->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(!f.match(type_int));
    QVERIFY(vf.match(var_a));
    QVERIFY(!vf.match(var_b));

    f.parseOption("size", QString::number(type_b->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(!f.match(type_a));
    QVERIFY(f.match(type_b));
    QVERIFY(!f.match(type_int));
    QVERIFY(!vf.match(var_a));
    QVERIFY(vf.match(var_b));

    f.parseOption("size", QString::number(type_int->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(f.match(type_int));
    QVERIFY(!vf.match(var_a));
    QVERIFY(!vf.match(var_b));

    f.parseOption("size", QString::number(
                      type_int->size() + type_a->size() + type_b->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(!f.match(type_a));
    QVERIFY(!f.match(type_b));
    QVERIFY(!f.match(type_int));
    QVERIFY(!vf.match(var_a));
    QVERIFY(!vf.match(var_b));
}


QTEST_MAIN(TypeFilterTest)

#include "tst_typefiltertest.moc"
