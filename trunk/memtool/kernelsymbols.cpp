/*
 * kernelsymbols.cpp
 *
 *  Created on: 30.03.2010
 *      Author: chrschn
 */

#include "kernelsymbols.h"

#include <QIODevice>
#include <QRegExp>
#include <QHash>
#include "typefactory.h"

#define parserError(x) do { throw ParserException((x), __FILE__, __LINE__); } while (0)


enum HdrSymbolType {
	hsUnknownSymbol = 0,  // Start with 0 for any unknown symbol
	hsCompileUnit,        // This represents first "real" symbol
	hsBaseType,
	hsArrayType,
	hsSubrangeType,
	hsPointerType,
	hsConstType,
	hsSubroutineType,
	hsTypedef,
	hsFormalParameter,
	hsStructureType,
	hsMember,
	hsVolatileType,
	hsUnionType,
	hsEnumerationType,
	hsEnumerator,
	hsSubprogram,
	hsVariable,
	hsLexicalBlock,
	hsInlinedSubroutine,
	hsUnspecifiedParameters,
	hsLabel
};

enum ParamSymbolType {
	psUnknownSymbol = 0,  // Start with 0 for any unknown symbol
	psRanges,             // This represents first "real" symbol
	psName,
	psCompDir,
	psProducer,
	psLanguage,
	psLowPc,
	psEntryPc,
	psStmtList,
	psByteSize,
	psEncoding,
	psType,
	psSibling,
	psUpperBound,
	psPrototyped,
	psDeclFile,
	psDeclLine,
	psDataMemberLocation,
	psDeclaration,
	psBitSize,
	psBitOffset,
	psConstValue,
	psInline,
	psAbstractOrigin,
	psExternal,
	psHighPc,
	psFrameBase,
	psLocation,
	psCallFile,
	psCallLine,
	psArtificial
};

typedef QHash<QString, HdrSymbolType> HdrSymMap;
typedef QHash<QString, ParamSymbolType> ParamSymMap;


class TypeInfo
{
public:
	TypeFactory::Encoding enc;
	QString name;
	int id;
	qint32 size;
	HdrSymbolType type;

	TypeInfo() { clear(); }

	void clear()
	{
		enc = TypeFactory::eUndef;
		name.clear();
		id = -1;
		size = -1;
		type = hsUnknownSymbol;
	}

	bool isValid() const
	{
		return !name.isEmpty() && id != -1 && size != -1 && type != hsUnknownSymbol;
	}

};


static HdrSymMap getHdrSymMap()
{
	HdrSymMap ret;
	ret.insert("DW_TAG_compile_unit", hsCompileUnit);
	ret.insert("DW_TAG_base_type", hsBaseType);
	ret.insert("DW_TAG_array_type", hsArrayType);
	ret.insert("DW_TAG_subrange_type", hsSubrangeType);
	ret.insert("DW_TAG_pointer_type", hsPointerType);
	ret.insert("DW_TAG_const_type", hsConstType);
	ret.insert("DW_TAG_subroutine_type", hsSubroutineType);
	ret.insert("DW_TAG_typedef", hsTypedef);
	ret.insert("DW_TAG_formal_parameter", hsFormalParameter);
	ret.insert("DW_TAG_structure_type", hsStructureType);
	ret.insert("DW_TAG_member", hsMember);
	ret.insert("DW_TAG_volatile_type", hsVolatileType);
	ret.insert("DW_TAG_union_type", hsUnionType);
	ret.insert("DW_TAG_enumeration_type", hsEnumerationType);
	ret.insert("DW_TAG_enumerator", hsEnumerator);
	ret.insert("DW_TAG_subprogram", hsSubprogram);
	ret.insert("DW_TAG_variable", hsVariable);
	ret.insert("DW_TAG_lexical_block", hsLexicalBlock);
	ret.insert("DW_TAG_inlined_subroutine", hsInlinedSubroutine);
	ret.insert("DW_TAG_unspecified_parameters", hsUnspecifiedParameters);
	ret.insert("DW_TAG_label", hsLabel);
	return ret;
}

static ParamSymMap getParamSymMap()
{
	ParamSymMap ret;
	ret.insert("DW_AT_ranges", psRanges);
	ret.insert("DW_AT_name", psName);
	ret.insert("DW_AT_comp_dir", psCompDir);
	ret.insert("DW_AT_producer", psProducer);
	ret.insert("DW_AT_language", psLanguage);
	ret.insert("DW_AT_low_pc", psLowPc);
	ret.insert("DW_AT_entry_pc", psEntryPc);
	ret.insert("DW_AT_stmt_list", psStmtList);
	ret.insert("DW_AT_byte_size", psByteSize);
	ret.insert("DW_AT_encoding", psEncoding);
	ret.insert("DW_AT_type", psType);
	ret.insert("DW_AT_sibling", psSibling);
	ret.insert("DW_AT_upper_bound", psUpperBound);
	ret.insert("DW_AT_prototyped", psPrototyped);
	ret.insert("DW_AT_decl_file", psDeclFile);
	ret.insert("DW_AT_decl_line", psDeclLine);
	ret.insert("DW_AT_data_member_location", psDataMemberLocation);
	ret.insert("DW_AT_declaration", psDeclaration);
	ret.insert("DW_AT_bit_size", psBitSize);
	ret.insert("DW_AT_bit_offset", psBitOffset);
	ret.insert("DW_AT_const_value", psConstValue);
	ret.insert("DW_AT_inline", psInline);
	ret.insert("DW_AT_abstract_origin", psAbstractOrigin);
	ret.insert("DW_AT_external", psExternal);
	ret.insert("DW_AT_high_pc", psHighPc);
	ret.insert("DW_AT_frame_base", psFrameBase);
	ret.insert("DW_AT_location", psLocation);
	ret.insert("DW_AT_call_file", psCallFile);
	ret.insert("DW_AT_call_line", psCallLine);
	ret.insert("DW_AT_artificial", psArtificial);
	return ret;
}


KernelSymbols::Parser::Parser(QIODevice* from, TypeFactory* factory)
	: _from(from), _factory(factory)
{
}

/*
 <0><b>: Abbrev Number: 1 (DW_TAG_compile_unit)
  < c>     DW_AT_producer    : (indirect string, offset: 0xb0): GNU C 4.3.4
  <10>     DW_AT_language    : 1	(ANSI C)
  <11>     DW_AT_name        : (indirect string, offset: 0x25): test.c
  <15>     DW_AT_comp_dir    : (indirect string, offset: 0x38): /home/chrschn
  <19>     DW_AT_low_pc      : 0x40053c
  <21>     DW_AT_high_pc     : 0x400566
  <29>     DW_AT_stmt_list   : 0
 <1><2d>: Abbrev Number: 2 (DW_TAG_base_type)
  <2e>     DW_AT_byte_size   : 8
  <2f>     DW_AT_encoding    : 7	(unsigned)
  <30>     DW_AT_name        : (indirect string, offset: 0x116): long unsigned int
 <1><34>: Abbrev Number: 2 (DW_TAG_base_type)
  <35>     DW_AT_byte_size   : 1
  <36>     DW_AT_encoding    : 8	(unsigned char)
  <37>     DW_AT_name        : (indirect string, offset: 0xbc): unsigned char
*/


void KernelSymbols::Parser::parse()
{
	const int bufSize = 4096;
	char buf[bufSize];

	static const char* hdrRegex = "^\\s*<[0-9a-f]+><([0-9a-f]+)>:[^(]+\\(([^)]+)\\)";
	static const char* paramRegex = "^\\s*<[0-9a-f]+>\\s*([^\\s]+)\\s*:\\s*(.*)\\s*$";

	QRegExp rxHdr(hdrRegex);
	QRegExp rxParam(paramRegex);

	HdrSymMap hdrMap = getHdrSymMap();
	ParamSymMap paramMap = getParamSymMap();

	bool ok;
	QString line, val;
	HdrSymbolType hdrSym = hsUnknownSymbol, prevHdrSym;
	ParamSymbolType paramSym;
	TypeInfo info;

	while (!_from->atEnd()) {
		int len = _from->readLine(buf, bufSize);
		// Skip all lines without interesting information
		if (
				len < 30 ||
				buf[0] != ' ' ||
				( buf[1] != '<' || (buf[1] != ' ' && buf[2] != '<') )
			)
		{
			continue;
		}

		line = buf;

		// First see if a new header starts
		if (rxHdr.exactMatch(line)) {

			// See if we have to finish something before we continue parsing
			switch(hdrSym) {
			case hsBaseType:
				// Did we parse all required parameters?
				if (!info.isValid()) {
					parserError(QString("Did not parse all required params: id=%1, size=%2, name=%3, type=%4")
							.arg(info.id)
							.arg(info.size)
							.arg(info.name)
							.arg(info.type));
				}
				// A BaseType also needs an encoding
				if (!info.enc != TypeFactory::eUndef) {
					parserError(QString("Did not parse an encoding for type %1").arg(info.name));
				}

//				BaseType* t =
				_factory->getNumericInstance(info.name, info.id, info.size, info.enc);
				break;

			default:
				break;
			}

			// Reset all data for a new type
			info.clear();

			info.id = rxHdr.cap(1).toInt(&ok, 16);
			if (!ok)
				parserError(QString("Illegal integer number: %1").arg(rxHdr.cap(1)));

			// Remember previous symbol
			prevHdrSym = hdrSym;
			// If the symbol does not exist in the hash, it will return 0, which
			// corresponds to hsUnknownSymbol.
			hdrSym = hdrMap.value(rxHdr.cap(2));
			if (hdrSym == hsUnknownSymbol)
				parserError(QString("Unknown debug symbol type encountered: %1").arg(rxHdr.cap(2)));

//			switch (hdrSym) {
//			case hsBaseType:
//			}
		}
		// Next see if this matches a parameter
		else if (rxParam.exactMatch(line)) {
			paramSym = paramMap.value(rxParam.cap(1));
			val = rxParam.cap(2);
		}

	}
}


//------------------------------------------------------------------------------
KernelSymbols::KernelSymbols()
{
	// TODO Auto-generated constructor stub

}


KernelSymbols::~KernelSymbols()
{
	// TODO Auto-generated destructor stub
}


void KernelSymbols::parseSymbols(QIODevice* from)
{
	if (!from)
		parserError("Received a null device to read the data from");
}
