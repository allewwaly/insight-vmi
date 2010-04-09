/*
 * typeinfo.cpp
 *
 *  Created on: 31.03.2010
 *      Author: chrschn
 */

#include "typeinfo.h"


TypeInfo::TypeInfo()
{
	clear();
}


void TypeInfo::clear()
{
	_enc = eUndef;
	_name.clear();
	_id = _refTypeId = -1;
	_byteSize = 0;
	_bitSize = _bitOffset = -1;
	_location = 0;
	_upperBound = -1;
	_constValue = -1;
	_symType = hsUnknownSymbol;
	_srcDir.clear();
	_srcFileId = _srcLine = -1;
	_enumValues.clear();
}


const QString& TypeInfo::name() const
{
	return _name;
}


void TypeInfo::setName(QString name)
{
    this->_name = name;
}


const QString& TypeInfo::srcDir() const
{
	return _srcDir;
}


void TypeInfo::setSrcDir(QString dir)
{
	_srcDir = dir;
}

int TypeInfo::srcFileId() const
{
	return _srcFileId;
}


void TypeInfo::setSrcFileId(int id)
{
	_srcFileId = id;
}


int TypeInfo::srcLine() const
{
	return _srcLine;
}


void TypeInfo::setSrcLine(int lineno)
{
	_srcLine = lineno;
}


int TypeInfo::id() const
{
    return _id;
}


void TypeInfo::setId(int id)
{
    this->_id = id;
}


int TypeInfo::refTypeId() const
{
    return _refTypeId;
}


void TypeInfo::setRefTypeId(int refTypeID)
{
    this->_refTypeId = refTypeID;
}


quint32 TypeInfo::byteSize() const
{
    return _byteSize;
}


void TypeInfo::setByteSize(quint32 byteSize)
{
    this->_byteSize = byteSize;
}


int TypeInfo::bitSize() const
{
    return _bitSize;
}


void TypeInfo::setBitSize(int bitSize)
{
    _bitSize = bitSize;
}


int TypeInfo::bitOffset() const
{
    return _bitOffset;
}


void TypeInfo::setBitOffset(int bitOffset)
{
    _bitOffset = bitOffset;
}


size_t TypeInfo::location() const
{
    return _location;
}


void TypeInfo::setLocation(size_t location)
{
    this->_location = location;
}


qint32 TypeInfo::dataMemberLoc() const
{
    return _dataMemberLoc;
}


void TypeInfo::setDataMemberLoc(qint32 location)
{
    this->_dataMemberLoc = location;
}


HdrSymbolType TypeInfo::symType() const
{
    return _symType;
}


void TypeInfo::setSymType(HdrSymbolType symType)
{
    this->_symType = symType;
}


DataEncoding TypeInfo::enc() const
{
    return _enc;
}


void TypeInfo::setEnc(DataEncoding enc)
{
    this->_enc = enc;
}


qint32 TypeInfo::upperBound() const
{
	return _upperBound;
}


void TypeInfo::setUpperBound(qint32 bound)
{
	_upperBound = bound;
}

qint32 TypeInfo::constValue() const
{
	return _constValue;
}


void TypeInfo::setConstValue(qint32 value)
{
	_constValue = value;
}


const TypeInfo::EnumHash& TypeInfo::enumValues() const
{
	return _enumValues;
}


void TypeInfo::addEnumValue(const QString& name, qint32 value)
{
	_enumValues.insert(value, name);
}


QString TypeInfo::dump() const
{
	static HdrSymMap hdrMap = getHdrSymMap();
	QString symType;
	for (HdrSymMap::iterator it = hdrMap.begin(); it != hdrMap.end(); ++it)
		if (_symType == it.value())
			symType = it.key();

	static DataEncMap encMap = getDataEncMap();
	QString enc;
	for (DataEncMap::iterator it = encMap.begin(); it != encMap.end(); ++it)
		if (_enc == it.value())
			enc = it.key();

	QString ret;
	if (_id >= 0) 		     ret += QString("  id:            0x%1\n").arg(_id, 0, 16);
	if (_symType >= 0)       ret += QString("  symType:       %1 (%2)\n").arg(_symType).arg(symType);
	if (!_name.isEmpty())    ret += QString("  name:          %1\n").arg(_name);
	if (_byteSize > 0)       ret += QString("  byteSize:      %1\n").arg(_byteSize);
    if (_bitSize >= 0)       ret += QString("  bitSize :      %1\n").arg(_bitSize);
    if (_bitOffset >= 0)     ret += QString("  bitOffset :    %1\n").arg(_bitOffset);
	if (_enc > 0)            ret += QString("  enc:           %1 (%2)\n").arg(_enc).arg(enc);
	if (_location > 0)       ret += QString("  location:      %1\n").arg(_location);
	if (_dataMemberLoc >= 0) ret += QString("  dataMemberLoc: %1\n").arg(_dataMemberLoc);
	if (_refTypeId >= 0)     ret += QString("  refType:       0x%1\n").arg(_refTypeId, 0, 16);
	if (_upperBound >= 0)    ret += QString("  upperBound:    %1\n").arg(_upperBound);
	if (!_srcDir.isEmpty())  ret += QString("  srcDir:        %1\n").arg(_srcDir);
	if (_srcFileId >= 0)     ret += QString("  srcFile:       %1\n").arg(_srcFileId);
	if (_srcLine >= 0)       ret += QString("  srcLine:       %1\n").arg(_srcLine);
	return ret;
}


//------------------------------------------------------------------------------
HdrSymMap getHdrSymMap()
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


ParamSymMap getParamSymMap()
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


DataEncMap getDataEncMap()
{
	DataEncMap ret;
	ret.insert("float", eFloat);
	ret.insert("signed", eSigned);
	ret.insert("boolean", eBoolean);
	ret.insert("unsigned", eUnsigned);
	return ret;
}

