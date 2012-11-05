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
        "    <c>   DW_AT_producer    : (indirect string, offset: 0x0): GNU C 4.5.4      \n"
        "    <10>   DW_AT_language    : 1       (ANSI C)\n"
        "    <11>   DW_AT_name        : (indirect string, offset: 0x2e): test.c \n"
        "    <15>   DW_AT_comp_dir    : (indirect string, offset: 0x43): /home/chrschn/workspace/insight-vmi/tests/typefilter   \n"
        "    <19>   DW_AT_low_pc      : 0x4004e4        \n"
        "    <21>   DW_AT_high_pc     : 0x400514        \n"
        "    <29>   DW_AT_stmt_list   : 0x0     \n"
        " <1><2d>: Abbrev Number: 2 (DW_TAG_structure_type)\n"
        "    <2e>   DW_AT_name        : A       \n"
        "    <30>   DW_AT_byte_size   : 24      \n"
        "    <31>   DW_AT_decl_file   : 1       \n"
        "    <32>   DW_AT_decl_line   : 1       \n"
        "    <33>   DW_AT_sibling     : <0x68>  \n"
        " <2><37>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <38>   DW_AT_name        : l       \n"
        "    <3a>   DW_AT_decl_file   : 1       \n"
        "    <3b>   DW_AT_decl_line   : 2       \n"
        "    <3c>   DW_AT_type        : <0x68>  \n"
        "    <40>   DW_AT_data_member_location: 2 byte block: 23 0      (DW_OP_plus_uconst: 0)\n"
        " <2><43>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <44>   DW_AT_name        : s       \n"
        "    <46>   DW_AT_decl_file   : 1       \n"
        "    <47>   DW_AT_decl_line   : 3       \n"
        "    <48>   DW_AT_type        : <0x6f>  \n"
        "    <4c>   DW_AT_data_member_location: 2 byte block: 23 8      (DW_OP_plus_uconst: 8)\n"
        " <2><4f>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <50>   DW_AT_name        : i       \n"
        "    <52>   DW_AT_decl_file   : 1       \n"
        "    <53>   DW_AT_decl_line   : 4       \n"
        "    <54>   DW_AT_type        : <0x76>  \n"
        "    <58>   DW_AT_data_member_location: 2 byte block: 23 c      (DW_OP_plus_uconst: 12)\n"
        " <2><5b>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <5c>   DW_AT_name        : c       \n"
        "    <5e>   DW_AT_decl_file   : 1       \n"
        "    <5f>   DW_AT_decl_line   : 5       \n"
        "    <60>   DW_AT_type        : <0x7d>  \n"
        "    <64>   DW_AT_data_member_location: 2 byte block: 23 10     (DW_OP_plus_uconst: 16)\n"
        " <1><68>: Abbrev Number: 4 (DW_TAG_base_type)\n"
        "    <69>   DW_AT_byte_size   : 8       \n"
        "    <6a>   DW_AT_encoding    : 5       (signed)\n"
        "    <6b>   DW_AT_name        : (indirect string, offset: 0x35): long int       \n"
        " <1><6f>: Abbrev Number: 4 (DW_TAG_base_type)\n"
        "    <70>   DW_AT_byte_size   : 2       \n"
        "    <71>   DW_AT_encoding    : 5       (signed)\n"
        "    <72>   DW_AT_name        : (indirect string, offset: 0x95): short int      \n"
        " <1><76>: Abbrev Number: 5 (DW_TAG_base_type)\n"
        "    <77>   DW_AT_byte_size   : 4       \n"
        "    <78>   DW_AT_encoding    : 5       (signed)\n"
        "    <79>   DW_AT_name        : int     \n"
        " <1><7d>: Abbrev Number: 4 (DW_TAG_base_type)\n"
        "    <7e>   DW_AT_byte_size   : 1       \n"
        "    <7f>   DW_AT_encoding    : 6       (signed char)\n"
        "    <80>   DW_AT_name        : (indirect string, offset: 0x29): char   \n"
        " <1><84>: Abbrev Number: 6 (DW_TAG_union_type)\n"
        "    <85>   DW_AT_byte_size   : 24      \n"
        "    <86>   DW_AT_decl_file   : 1       \n"
        "    <87>   DW_AT_decl_line   : 14      \n"
        "    <88>   DW_AT_sibling     : <0xae>  \n"
        " <2><8c>: Abbrev Number: 7 (DW_TAG_member)\n"
        "    <8d>   DW_AT_name        : (indirect string, offset: 0x9f): nested_i       \n"
        "    <91>   DW_AT_decl_file   : 1       \n"
        "    <92>   DW_AT_decl_line   : 15      \n"
        "    <93>   DW_AT_type        : <0x76>  \n"
        " <2><97>: Abbrev Number: 7 (DW_TAG_member)\n"
        "    <98>   DW_AT_name        : (indirect string, offset: 0x8c): nested_f       \n"
        "    <9c>   DW_AT_decl_file   : 1       \n"
        "    <9d>   DW_AT_decl_line   : 16      \n"
        "    <9e>   DW_AT_type        : <0xae>  \n"
        " <2><a2>: Abbrev Number: 7 (DW_TAG_member)\n"
        "    <a3>   DW_AT_name        : (indirect string, offset: 0x78): nested_a       \n"
        "    <a7>   DW_AT_decl_file   : 1       \n"
        "    <a8>   DW_AT_decl_line   : 17      \n"
        "    <a9>   DW_AT_type        : <0x2d>  \n"
        " <1><ae>: Abbrev Number: 4 (DW_TAG_base_type)\n"
        "    <af>   DW_AT_byte_size   : 4       \n"
        "    <b0>   DW_AT_encoding    : 4       (float)\n"
        "    <b1>   DW_AT_name        : (indirect string, offset: 0x86): float  \n"
        " <1><b5>: Abbrev Number: 2 (DW_TAG_structure_type)\n"
        "    <b6>   DW_AT_name        : B       \n"
        "    <b8>   DW_AT_byte_size   : 168     \n"
        "    <b9>   DW_AT_decl_file   : 1       \n"
        "    <ba>   DW_AT_decl_line   : 8       \n"
        "    <bb>   DW_AT_sibling     : <0x10b> \n"
        " <2><bf>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <c0>   DW_AT_name        : j       \n"
        "    <c2>   DW_AT_decl_file   : 1       \n"
        "    <c3>   DW_AT_decl_line   : 9       \n"
        "    <c4>   DW_AT_type        : <0x76>  \n"
        "    <c8>   DW_AT_data_member_location: 2 byte block: 23 0      (DW_OP_plus_uconst: 0)\n"
        " <2><cb>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <cc>   DW_AT_name        : a       \n"
        "    <ce>   DW_AT_decl_file   : 1       \n"
        "    <cf>   DW_AT_decl_line   : 10      \n"
        "    <d0>   DW_AT_type        : <0x2d>  \n"
        "    <d4>   DW_AT_data_member_location: 2 byte block: 23 8      (DW_OP_plus_uconst: 8)\n"
        " <2><d7>: Abbrev Number: 8 (DW_TAG_member)\n"
        "    <d8>   DW_AT_name        : (indirect string, offset: 0x1e): array  \n"
        "    <dc>   DW_AT_decl_file   : 1       \n"
        "    <dd>   DW_AT_decl_line   : 11      \n"
        "    <de>   DW_AT_type        : <0x10b> \n"
        "    <e2>   DW_AT_data_member_location: 2 byte block: 23 20     (DW_OP_plus_uconst: 32)\n"
        " <2><e5>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <e6>   DW_AT_name        : pa      \n"
        "    <e9>   DW_AT_decl_file   : 1       \n"
        "    <ea>   DW_AT_decl_line   : 12      \n"
        "    <eb>   DW_AT_type        : <0x122> \n"
        "    <ef>   DW_AT_data_member_location: 3 byte block: 23 80 1   (DW_OP_plus_uconst: 128)\n"
        " <2><f3>: Abbrev Number: 3 (DW_TAG_member)\n"
        "    <f4>   DW_AT_name        : pb      \n"
        "    <f7>   DW_AT_decl_file   : 1       \n"
        "    <f8>   DW_AT_decl_line   : 13      \n"
        "    <f9>   DW_AT_type        : <0x128> \n"
        "    <fd>   DW_AT_data_member_location: 3 byte block: 23 88 1   (DW_OP_plus_uconst: 136)\n"
        " <2><101>: Abbrev Number: 9 (DW_TAG_member)\n"
        "    <102>   DW_AT_type        : <0x84> \n"
        "    <106>   DW_AT_data_member_location: 3 byte block: 23 90 1  (DW_OP_plus_uconst: 144)\n"
        " <1><10b>: Abbrev Number: 10 (DW_TAG_array_type)\n"
        "    <10c>   DW_AT_type        : <0x2d> \n"
        "    <110>   DW_AT_sibling     : <0x11b>        \n"
        " <2><114>: Abbrev Number: 11 (DW_TAG_subrange_type)\n"
        "    <115>   DW_AT_type        : <0x11b>        \n"
        "    <119>   DW_AT_upper_bound : 3      \n"
        " <1><11b>: Abbrev Number: 4 (DW_TAG_base_type)\n"
        "    <11c>   DW_AT_byte_size   : 8      \n"
        "    <11d>   DW_AT_encoding    : 7      (unsigned)\n"
        "    <11e>   DW_AT_name        : (indirect string, offset: 0xc): long unsigned int      \n"
        " <1><122>: Abbrev Number: 12 (DW_TAG_pointer_type)\n"
        "    <123>   DW_AT_byte_size   : 8      \n"
        "    <124>   DW_AT_type        : <0x2d> \n"
        " <1><128>: Abbrev Number: 12 (DW_TAG_pointer_type)\n"
        "    <129>   DW_AT_byte_size   : 8      \n"
        "    <12a>   DW_AT_type        : <0xb5> \n"
        " <1><12e>: Abbrev Number: 13 (DW_TAG_subprogram)\n"
        "    <12f>   DW_AT_external    : 1      \n"
        "    <130>   DW_AT_name        : (indirect string, offset: 0x24): main  \n"
        "    <134>   DW_AT_decl_file   : 1      \n"
        "    <135>   DW_AT_decl_line   : 24     \n"
        "    <136>   DW_AT_prototyped  : 1      \n"
        "    <137>   DW_AT_type        : <0x76> \n"
        "    <13b>   DW_AT_low_pc      : 0x4004e4       \n"
        "    <143>   DW_AT_high_pc     : 0x400514       \n"
        "    <14b>   DW_AT_frame_base  : 0x0    (location list)\n"
        "    <14f>   DW_AT_sibling     : <0x170>        \n"
        " <2><153>: Abbrev Number: 14 (DW_TAG_formal_parameter)\n"
        "    <154>   DW_AT_name        : (indirect string, offset: 0x3e): argc  \n"
        "    <158>   DW_AT_decl_file   : 1      \n"
        "    <159>   DW_AT_decl_line   : 24     \n"
        "    <15a>   DW_AT_type        : <0x76> \n"
        "    <15e>   DW_AT_location    : 2 byte block: 91 6c    (DW_OP_fbreg: -20)\n"
        " <2><161>: Abbrev Number: 14 (DW_TAG_formal_parameter)\n"
        "    <162>   DW_AT_name        : (indirect string, offset: 0x81): argv  \n"
        "    <166>   DW_AT_decl_file   : 1      \n"
        "    <167>   DW_AT_decl_line   : 24     \n"
        "    <168>   DW_AT_type        : <0x170>        \n"
        "    <16c>   DW_AT_location    : 2 byte block: 91 60    (DW_OP_fbreg: -32)\n"
        " <1><170>: Abbrev Number: 12 (DW_TAG_pointer_type)\n"
        "    <171>   DW_AT_byte_size   : 8      \n"
        "    <172>   DW_AT_type        : <0x176>        \n"
        " <1><176>: Abbrev Number: 12 (DW_TAG_pointer_type)\n"
        "    <177>   DW_AT_byte_size   : 8      \n"
        "    <178>   DW_AT_type        : <0x7d> \n"
        " <1><17c>: Abbrev Number: 15 (DW_TAG_variable)\n"
        "    <17d>   DW_AT_name        : a      \n"
        "    <17f>   DW_AT_decl_file   : 1      \n"
        "    <180>   DW_AT_decl_line   : 21     \n"
        "    <181>   DW_AT_type        : <0x2d> \n"
        "    <185>   DW_AT_external    : 1      \n"
        "    <186>   DW_AT_declaration : 1      \n"
        " <1><187>: Abbrev Number: 15 (DW_TAG_variable)\n"
        "    <188>   DW_AT_name        : b      \n"
        "    <18a>   DW_AT_decl_file   : 1      \n"
        "    <18b>   DW_AT_decl_line   : 22     \n"
        "    <18c>   DW_AT_type        : <0xb5> \n"
        "    <190>   DW_AT_external    : 1      \n"
        "    <191>   DW_AT_declaration : 1      \n"
        " <1><192>: Abbrev Number: 16 (DW_TAG_variable)\n"
        "    <193>   DW_AT_name        : a      \n"
        "    <195>   DW_AT_decl_file   : 1      \n"
        "    <196>   DW_AT_decl_line   : 21     \n"
        "    <197>   DW_AT_type        : <0x2d> \n"
        "    <19b>   DW_AT_external    : 1      \n"
        "    <19c>   DW_AT_location    : 9 byte block: 3 f0 10 60 0 0 0 0 0     (DW_OP_addr: 6010f0)\n"
        " <1><1a6>: Abbrev Number: 16 (DW_TAG_variable)\n"
        "    <1a7>   DW_AT_name        : b      \n"
        "    <1a9>   DW_AT_decl_file   : 1      \n"
        "    <1aa>   DW_AT_decl_line   : 22     \n"
        "    <1ab>   DW_AT_type        : <0xb5> \n"
        "    <1af>   DW_AT_external    : 1      \n"
        "    <1b0>   DW_AT_location    : 9 byte block: 3 40 10 60 0 0 0 0 0     (DW_OP_addr: 601040)\n";

#define VERIFY_F_VF(ta, tb, ti, va, vb) \
    QVERIFY(f.matchType(type_a) == (ta)); \
    QVERIFY(f.matchType(type_b) == (tb)); \
    QVERIFY(f.matchType(type_int) == (ti)); \
    QVERIFY(vf.matchVar(var_a) == (va)); \
    QVERIFY(vf.matchVar(var_b) == (vb));


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
    void parseMembers();

    void setDataType();
    void setTypeName();
    void setVarName();
    void setFileName();
    void setSize();
    void setMembers();

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
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("datatype", "struct,union");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("datatype", "*int*,struct,union");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("datatype", "str*");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("datatype", "*int*");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("datatype", "/int/");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("datatype", "/U?[Ii]nt/");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("datatype", "/Int|Struct/");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("datatype", "/int/i");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    // Whitespace
    f.parseOption("datatype", "  struct");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("datatype", "struct  ");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("datatype", "  struct  ");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    // Case sensitivity
    f.parseOption("datatype", "StRuCt");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));
}


void TypeFilterTest::parseTypeName()
{
    TypeFilter f;

    f.parseOption("typename", "A");
    QVERIFY(f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "a");
    QVERIFY(f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "B");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "b");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "int");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("typename", "  int");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("typename", "int  ");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("typename", "  int  ");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("typename", "InT");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("typename", "*a*");
    QVERIFY(f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "*i*");
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));

    f.parseOption("typename", "/[AB]/");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "/[Ab]/");
    QVERIFY(f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));

    f.parseOption("typename", "/[Ab]/i");
    QVERIFY(f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));
}


void TypeFilterTest::parseVarName()
{
    VariableFilter f;

    f.parseOption("variablename", "A");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "a");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "B");
    QVERIFY(!f.matchVar(var_a));
    QVERIFY(f.matchVar(var_b));

    f.parseOption("variablename", "b");
    QVERIFY(!f.matchVar(var_a));
    QVERIFY(f.matchVar(var_b));

    f.parseOption("variablename", "/A/");
    QVERIFY(!f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "/A/i");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "/a/");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "/^a$/");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "*A*");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "*a*");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "  A");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "A  ");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "  A  ");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));

    f.parseOption("variablename", "/[ab]/");
    QVERIFY(f.matchVar(var_a));
    QVERIFY(f.matchVar(var_b));

    f.parseOption("variablename", "foo");
    QVERIFY(!f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));
}


void TypeFilterTest::parseFileName()
{
    VariableFilter f;

    // Cannot be tested, requires further changes in KernelSymbolParser
    f.parseOption("filename", "vmlinux");
    QVERIFY(!f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));
}


void TypeFilterTest::parseSize()
{
    TypeFilter f;
    VariableFilter vf;

    f.parseOption("size", QString::number(type_a->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));
    QVERIFY(vf.matchVar(var_a));
    QVERIFY(!vf.matchVar(var_b));

    f.parseOption("size", QString::number(type_b->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(!f.matchType(type_a));
    QVERIFY(f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));
    QVERIFY(!vf.matchVar(var_a));
    QVERIFY(vf.matchVar(var_b));

    f.parseOption("size", QString::number(type_int->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(f.matchType(type_int));
    QVERIFY(!vf.matchVar(var_a));
    QVERIFY(!vf.matchVar(var_b));

    f.parseOption("size", QString::number(
                      type_int->size() + type_a->size() + type_b->size()));
    vf.parseOption("size", QString::number(f.size()));
    QVERIFY(!f.matchType(type_a));
    QVERIFY(!f.matchType(type_b));
    QVERIFY(!f.matchType(type_int));
    QVERIFY(!vf.matchVar(var_a));
    QVERIFY(!vf.matchVar(var_b));
}


void TypeFilterTest::parseMembers()
{
    TypeFilter f;
    VariableFilter vf;

#define TEST_PARSE_MEMBER1(f1, kv, ta, tb, va, vb) \
    f.clear(); \
    vf.clear(); \
    f.parseOption("member", f1, kv); \
    vf.parseOption("member", f1, kv); \
    QVERIFY(f.members().size() == 1); \
    QVERIFY(vf.members().size() == 1); \
    VERIFY_F_VF(ta, tb, false, va, vb)

#define TEST_PARSE_MEMBER2(f1, f2, kv, ta, tb, va, vb) \
    f.clear(); \
    vf.clear(); \
    f.parseOption("member", f1, kv); \
    vf.parseOption("member", f1, kv); \
    f.parseOption("member", f2, kv); \
    vf.parseOption("member", f2, kv); \
    QVERIFY(f.members().size() == 2); \
    QVERIFY(vf.members().size() == 2); \
    VERIFY_F_VF(ta, tb, false, va, vb)

#define TEST_PARSE_MEMBER3(f1, f2, f3, kv, ta, tb, va, vb) \
    f.clear(); \
    vf.clear(); \
    f.parseOption("member", f1, kv); \
    vf.parseOption("member", f1, kv); \
    f.parseOption("member", f2, kv); \
    vf.parseOption("member", f2, kv); \
    f.parseOption("member", f3, kv); \
    vf.parseOption("member", f3, kv); \
    QVERIFY(f.members().size() == 3); \
    QVERIFY(vf.members().size() == 3); \
    VERIFY_F_VF(ta, tb, false, va, vb)

    // Match none
    TEST_PARSE_MEMBER1("not_existing", 0, false, false, false, false);
    // struct A
    TEST_PARSE_MEMBER1("l", 0, true, false, true, false);
    TEST_PARSE_MEMBER1("s", 0, true, false, true, false);
    TEST_PARSE_MEMBER1("i", 0, true, false, true, false);
    TEST_PARSE_MEMBER1("c", 0, true, false, true, false);
    // struct B
    TEST_PARSE_MEMBER1("j", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("a", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("pa", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("array", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("nested_i", 0, false, false, false, false);
    // struct B nested
    TEST_PARSE_MEMBER2("a", "l", 0, false, true, false, true);
    TEST_PARSE_MEMBER2("a", "s", 0, false, true, false, true);
    TEST_PARSE_MEMBER2("a", "i", 0, false, true, false, true);
    TEST_PARSE_MEMBER2("a", "c", 0, false, true, false, true);
    TEST_PARSE_MEMBER2("a", "not_existing", 0, false, false, false, false);
    TEST_PARSE_MEMBER2("", "nested_i", 0, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_f", 0, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_a", 0, false, true, false, true);
    // struct B 2x nested
    TEST_PARSE_MEMBER3("", "nested_a", "l", 0, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "s", 0, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "i", 0, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "c", 0, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "", 0, false, false, false, false);
    TEST_PARSE_MEMBER3("", "nested_a", "not_existing", 0, false, false, false, false);

    // whitespace
    TEST_PARSE_MEMBER1("  a", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("a  ", 0, false, true, false, true);
    TEST_PARSE_MEMBER1(" a ", 0, false, true, false, true);
    // case-insensitive
    TEST_PARSE_MEMBER1("aRrAy", 0, false, true, false, true);
    TEST_PARSE_MEMBER1("ARRAY", 0, false, true, false, true);

    KeyValueStore kv;
    kv["match"] = "wildcard";

    // Match none
    TEST_PARSE_MEMBER1("not_existing", &kv, false, false, false, false);
    // struct A
    TEST_PARSE_MEMBER1("*l", &kv, true, false, true, false);
    TEST_PARSE_MEMBER1("*s*", &kv, true, false, true, false);
    TEST_PARSE_MEMBER1("?", &kv, true, true, true, true);
    TEST_PARSE_MEMBER1("c", &kv, true, false, true, false);
    TEST_PARSE_MEMBER1("C", &kv, true, false, true, false);
    // struct B
    TEST_PARSE_MEMBER1("p?", &kv, false, true, false, true);
    TEST_PARSE_MEMBER1("p*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER1("nested_*", &kv, false, false, false, false);
    // struct B nested
    TEST_PARSE_MEMBER2("a", "?", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("?", "?", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("*", "*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("?", "nested_*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER2("*", "nested_*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("*", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_?", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_f", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_x", &kv, false, false, false, false);
    // struct B 2x nested
    TEST_PARSE_MEMBER3("*", "*", "*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("*", "*", "?", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("*", "*", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "*", "*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "*", "?", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "*", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("foobar", "*", "*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("foobar", "*", "?", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("foobar", "*", "l", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("", "nested_*", "s", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_?", "i", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "c", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "nested_*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("", "nested_a", "not_existing", &kv, false, false, false, false);

    kv["match"] = "regex";

    // Match none
    TEST_PARSE_MEMBER1("not_existing", &kv, false, false, false, false);
    // struct A
    TEST_PARSE_MEMBER1(".*l", &kv, true, false, true, false);
    TEST_PARSE_MEMBER1("^.*s.*$", &kv, true, false, true, false);
    TEST_PARSE_MEMBER1("l?", &kv, true, true, true, true);
    TEST_PARSE_MEMBER1("c", &kv, true, false, true, false);
    TEST_PARSE_MEMBER1("C", &kv, false, false, false, false);
    // struct B
    TEST_PARSE_MEMBER1("p.", &kv, false, true, false, true);
    TEST_PARSE_MEMBER1("p.*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER1("nested_.", &kv, false, false, false, false);
    // struct B nested
    TEST_PARSE_MEMBER2("a", ".", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2(".", ".", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2(".*", ".*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2(".", "nested_.*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER2(".", "nested_.*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER2(".*", "nested_.*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2(".", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2(".*", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_.*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_.", &kv, false, true, false, true);
    TEST_PARSE_MEMBER2("", "nested_f", &kv, false, true, false, true);
    // struct B 2x nested
    TEST_PARSE_MEMBER3(".*", ".*", ".*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3(".*", ".*", ".", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3(".*", ".*", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", ".*", ".*", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", ".*", ".", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", ".*", "l", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("foobar", ".*", ".*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("foobar", ".*", ".", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("foobar", ".*", "l", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("", "nested_.*", "s", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_.", "i", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "c", &kv, false, true, false, true);
    TEST_PARSE_MEMBER3("", "nested_a", "nested_.*", &kv, false, false, false, false);
    TEST_PARSE_MEMBER3("", "nested_a", "not_existing", &kv, false, false, false, false);
}


void TypeFilterTest::setDataType()
{
    TypeFilter f;
    VariableFilter vf;

#define TEST_DATA_TYPE(t, ta, tb, ti, va, vb) \
    f.setDataType(t); \
    vf.setDataType(t); \
    QVERIFY(f.dataType() == (t)); \
    QVERIFY(vf.dataType() == (t)); \
    VERIFY_F_VF(ta, tb, ti, va, vb)

    TEST_DATA_TYPE(rtStruct, true, true, false, true, true);
    TEST_DATA_TYPE(rtInt32, false, false, true, false, false);
    TEST_DATA_TYPE(rtStruct|rtInt32, true, true, true, true, true);
    TEST_DATA_TYPE(rtUnion|rtTypedef, false, false, false, false, false);
    TEST_DATA_TYPE(rtInt32|rtTypedef|rtUnion, false, false, true, false, false);
    TEST_DATA_TYPE(rtStruct|rtPointer|rtInt8, true, true, false, true, true);
}


void TypeFilterTest::setTypeName()
{
    TypeFilter f;
    VariableFilter vf;

#define TEST_TYPE_NAME(n, s, ta, tb, ti, va, vb) \
    f.setTypeName(n, s); \
    vf.setTypeName(n, s); \
    QCOMPARE(f.typeName(), QString(n)); \
    QCOMPARE(f.typeNameSyntax(), (s)); \
    QCOMPARE(vf.typeName(), QString(n)); \
    QCOMPARE(vf.typeNameSyntax(), (s)); \
    VERIFY_F_VF(ta, tb, ti, va, vb)

#define TEST_TYPE_NAME_ANY(n, s, ta, tb, ti, va, vb) \
    f.setTypeName(n, s); \
    vf.setTypeName(n, s); \
    QCOMPARE(f.typeName(), QString()); \
    QCOMPARE(f.typeNameSyntax(), (s)); \
    QCOMPARE(vf.typeName(), QString()); \
    QCOMPARE(vf.typeNameSyntax(), (s)); \
    VERIFY_F_VF(ta, tb, ti, va, vb)

    TEST_TYPE_NAME("A", Filter::psLiteral, true, false, false, true, false);
    TEST_TYPE_NAME("a", Filter::psLiteral, true, false, false, true, false);
    TEST_TYPE_NAME("B", Filter::psLiteral, false, true, false, false, true);
    TEST_TYPE_NAME("b", Filter::psLiteral, false, true, false, false, true);
    TEST_TYPE_NAME("int", Filter::psLiteral, false, false, true, false, false);
    TEST_TYPE_NAME("InT", Filter::psLiteral, false, false, true, false, false);
    TEST_TYPE_NAME("z", Filter::psLiteral, false, false, false, false, false);

    TEST_TYPE_NAME("A", Filter::psWildcard, true, false, false, true, false);
    TEST_TYPE_NAME("A*", Filter::psWildcard, true, false, false, true, false);
    TEST_TYPE_NAME("[a]", Filter::psWildcard, true, false, false, true, false);
    TEST_TYPE_NAME("?", Filter::psWildcard, true, true, false, true, true);
    TEST_TYPE_NAME("[a-z]", Filter::psWildcard, true, true, false, true, true);
    TEST_TYPE_NAME("*b*", Filter::psWildcard, false, true, false, false, true);
    TEST_TYPE_NAME("*in?", Filter::psWildcard, false, false, true, false, false);
    TEST_TYPE_NAME("?nT*", Filter::psWildcard, false, false, true, false, false);
    TEST_TYPE_NAME("*z*", Filter::psWildcard, false, false, false, false, false);

    TEST_TYPE_NAME("A", Filter::psRegExp, true, false, false, true, false);
    TEST_TYPE_NAME("a", Filter::psRegExp, false, false, false, false, false);
    TEST_TYPE_NAME("B", Filter::psRegExp, false, true, false, false, true);
    TEST_TYPE_NAME("b", Filter::psRegExp, false, false, false, false, false);
    TEST_TYPE_NAME("int", Filter::psRegExp, false, false, true, false, false);
    TEST_TYPE_NAME("InT", Filter::psRegExp, false, false, false, false, false);
    TEST_TYPE_NAME("[ABab]", Filter::psRegExp, true, true, false, true, true);
    TEST_TYPE_NAME("A|B", Filter::psRegExp, true, true, false, true, true);
    TEST_TYPE_NAME("[z].*", Filter::psRegExp, false, false, false, false, false);

    TEST_TYPE_NAME_ANY("", Filter::psAny, true, true, true, true, true);
    TEST_TYPE_NAME_ANY("a", Filter::psAny, true, true, true, true, true);
    TEST_TYPE_NAME_ANY("B", Filter::psAny, true, true, true, true, true);
    TEST_TYPE_NAME_ANY("b", Filter::psAny, true, true, true, true, true);
    TEST_TYPE_NAME_ANY("int", Filter::psAny, true, true, true, true, true);
    TEST_TYPE_NAME_ANY(QString(), Filter::psAny, true, true, true, true, true);
}


void TypeFilterTest::setVarName()
{
    VariableFilter vf;

#define TEST_VAR_NAME(n, s, va, vb) \
    vf.setVarName(n, s); \
    QCOMPARE(vf.varName(), QString(n)); \
    QCOMPARE((int)vf.varNameSyntax(), (int)(s)); \
    QCOMPARE(vf.matchVar(var_a), (va)); \
    QCOMPARE(vf.matchVar(var_b), (vb));

#define TEST_VAR_NAME_ANY(n, s, va, vb) \
    vf.setVarName(n, s); \
    QCOMPARE(vf.varName(), QString()); \
    QCOMPARE((int)vf.varNameSyntax(), (int)(s)); \
    QCOMPARE(vf.matchVar(var_a), (va)); \
    QCOMPARE(vf.matchVar(var_b), (vb));

    TEST_VAR_NAME("a", Filter::psLiteral, true, false);
    TEST_VAR_NAME("A", Filter::psLiteral, true, false);
    TEST_VAR_NAME("b", Filter::psLiteral, false, true);
    TEST_VAR_NAME("B", Filter::psLiteral, false, true);
    TEST_VAR_NAME("z", Filter::psLiteral, false, false);

    TEST_VAR_NAME("a", Filter::psWildcard, true, false);
    TEST_VAR_NAME("*a*", Filter::psWildcard, true, false);
    TEST_VAR_NAME("*A*", Filter::psWildcard, true, false);
    TEST_VAR_NAME("*b*", Filter::psWildcard, false, true);
    TEST_VAR_NAME("*B*", Filter::psWildcard, false, true);
    TEST_VAR_NAME("?", Filter::psWildcard, true, true);
    TEST_VAR_NAME("z", Filter::psWildcard, false, false);

    TEST_VAR_NAME("a+", Filter::psRegExp, true, false);
    TEST_VAR_NAME("A+", Filter::psRegExp, false, false);
    TEST_VAR_NAME("[b]", Filter::psRegExp, false, true);
    TEST_VAR_NAME("[B]", Filter::psRegExp, false, false);
    TEST_VAR_NAME("^(a|b)$", Filter::psRegExp, true, true);
    TEST_VAR_NAME("^[ab]$", Filter::psRegExp, true, true);
    TEST_VAR_NAME(".", Filter::psRegExp, true, true);
    TEST_VAR_NAME("z", Filter::psRegExp, false, false);

    TEST_VAR_NAME_ANY("", Filter::psAny, true, true);
    TEST_VAR_NAME_ANY(QString(), Filter::psAny, true, true);
    TEST_VAR_NAME_ANY("a", Filter::psAny, true, true);
    TEST_VAR_NAME_ANY("z", Filter::psAny, true, true);
}


void TypeFilterTest::setFileName()
{
    VariableFilter f;

    // Cannot be tested, requires further changes in KernelSymbolParser
    f.setSymFileIndex(3);
    QVERIFY(!f.matchVar(var_a));
    QVERIFY(!f.matchVar(var_b));
}


void TypeFilterTest::setSize()
{
    TypeFilter f;
    VariableFilter vf;

#define TEST_SIZE(s, ta, tb, ti, va, vb) \
    f.setSize(s); \
    vf.setSize(s); \
    QVERIFY(f.size() == (s)); \
    QVERIFY(vf.size() == (s)); \
    VERIFY_F_VF(ta, tb, ti, va, vb)

    TEST_SIZE(type_a->size(), true, false, false, true, false);
    TEST_SIZE(type_b->size(), false, true, false, false, true);
    TEST_SIZE(type_int->size(), false, false, true, false, false);
    TEST_SIZE(type_a->size() + type_b->size() + type_int->size(),
              false, false, false, false, false);
}


void TypeFilterTest::setMembers()
{
    TypeFilter f;
    VariableFilter vf;
    MemberFilterList fl;

#define TEST_SET_MEMBER1(f1, s1, ta, tb) \
    f.clear(); \
    vf.clear(); \
    fl.clear(); \
    fl.append(MemberFilter(f1, s1)); \
    f.setMembers(fl); \
    vf.setMembers(fl); \
    QVERIFY(f.members().size() == 1); \
    QVERIFY(vf.members().size() == 1); \
    VERIFY_F_VF(ta, tb, false, ta, tb)

#define TEST_SET_MEMBER2(f1, s1, f2, s2, ta, tb) \
    f.clear(); \
    vf.clear(); \
    fl.clear(); \
    fl.append(MemberFilter(f1, s1)); \
    fl.append(MemberFilter(f2, s2)); \
    f.setMembers(fl); \
    vf.setMembers(fl); \
    QVERIFY(f.members().size() == 2); \
    QVERIFY(vf.members().size() == 2); \
    VERIFY_F_VF(ta, tb, false, ta, tb)

    TEST_SET_MEMBER1("l", Filter::psLiteral, true, false);
    TEST_SET_MEMBER1("a", Filter::psLiteral, false, true);
    TEST_SET_MEMBER1("", Filter::psLiteral, false, true);
    TEST_SET_MEMBER1(".", Filter::psRegExp, true, true);
    TEST_SET_MEMBER1("..", Filter::psRegExp, false, true);
    TEST_SET_MEMBER1("?", Filter::psWildcard, true, true);
    TEST_SET_MEMBER1("??", Filter::psWildcard, false, true);
    TEST_SET_MEMBER1("", Filter::psAny, true, true);
    TEST_SET_MEMBER1(QString(), Filter::psAny, true, true);

    TEST_SET_MEMBER2("?", Filter::psWildcard, "l", Filter::psLiteral, false, true);
    TEST_SET_MEMBER2("??", Filter::psWildcard, "l", Filter::psLiteral, false, false);
    TEST_SET_MEMBER2("?", Filter::psWildcard, "x?i", Filter::psRegExp, false, true);
    TEST_SET_MEMBER2("??", Filter::psWildcard, "p?a", Filter::psRegExp, false, false);
    TEST_SET_MEMBER2("^a?$", Filter::psRegExp, "^.*i$", Filter::psRegExp, false, true);
    TEST_SET_MEMBER2("^$", Filter::psRegExp, "nested_*", Filter::psWildcard, false, true);

    TEST_SET_MEMBER2("", Filter::psAny, "l", Filter::psLiteral, false, true);
    TEST_SET_MEMBER2("", Filter::psAny, "x?i", Filter::psRegExp, false, true);
    TEST_SET_MEMBER2("", Filter::psAny, "p?a", Filter::psRegExp, false, true);
    TEST_SET_MEMBER2("", Filter::psAny, "^.*i$", Filter::psRegExp, false, true);
    TEST_SET_MEMBER2("", Filter::psAny, "?", Filter::psWildcard, false, true);
    TEST_SET_MEMBER2("", Filter::psAny, "nested_*", Filter::psWildcard, false, true);
    TEST_SET_MEMBER2("", Filter::psAny, "", Filter::psAny, false, true);
}


QTEST_MAIN(TypeFilterTest)

#include "tst_typefiltertest.moc"
