/*
 * typeinfo.cpp
 *
 *  Created on: 31.03.2010
 *      Author: chrschn
 */

#include <insight/typeinfo.h>
#include <insight/structuredmember.h>
#include <insight/funcparam.h>
#include <QtAlgorithms>

TypeInfo::TypeInfo(int threadIndex)
	: _fileIndex(threadIndex)
{
	clear();
}


void TypeInfo::clear()
{
	_isRelevant = false;
	_enc = eUndef;
	_name.clear();
	_id = _origId = _refTypeId = 0;
	_byteSize = 0;
	_bitSize = _bitOffset = -1;
	_location = 0;
	_hasLocation = false;
	_dataMemberLoc = -1;
	_upperBounds.clear();
	_external = 0;
	_sibling = -1;
	_inlined = false;
	_pcLow = 0;
	_pcHigh = 0;
	_constValue = -1;
	_symType = hsUnknownSymbol;
	_srcDir.clear();
	_srcFileId = 0;
	_srcLine = -1;
	_enumValues.clear();
	_members.clear();
	_params.clear();
	_section.clear();
}


void TypeInfo::deleteParams()
{
	for (int i = 0; i < _params.size(); ++i)
		delete _params[i];
	_params.clear();
}


QString TypeInfo::dump(const QStringList& symFiles) const
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

	QStringList ub;
	for (int i = 0; i < _upperBounds.size(); ++i)
		ub.append(QString::number(_upperBounds[i]));

	QString ret;
	if (_id != 0) 		        ret += QString("  id:            0x%1\n").arg(_id, 0, 16);
	if (_origId != 0) 		    ret += QString("  origId:        0x%1\n").arg(_origId, 0, 16);
	if (_symType >= 0)          ret += QString("  symType:       %1 (%2)\n").arg(_symType).arg(symType);
	if (!_name.isEmpty())       ret += QString("  name:          %1\n").arg(_name);
	if (_byteSize > 0)          ret += QString("  byteSize:      %1\n").arg(_byteSize);
    if (_bitSize >= 0)          ret += QString("  bitSize :      %1\n").arg(_bitSize);
    if (_bitOffset >= 0)        ret += QString("  bitOffset :    %1\n").arg(_bitOffset);
	if (_enc > 0)               ret += QString("  enc:           %1 (%2)\n").arg(_enc).arg(enc);
	if (_location > 0)          ret += QString("  location:      %1\n").arg(_location);
	if (!_section.isEmpty())    ret += QString("  segment:       %1\n").arg(_section);
	if (_dataMemberLoc >= 0)    ret += QString("  dataMemberLoc: %1\n").arg(_dataMemberLoc);
	if (_refTypeId != 0)        ret += QString("  refTypeId:     0x%1\n").arg(_refTypeId, 0, 16);
	if (!ub.isEmpty())          ret += QString("  upperBound(s): %1\n").arg(ub.join(", "));
	if (_external)              ret += QString("  external:      %1\n").arg(_external);
	if (_inlined)               ret += QString("  inlined:       %1\n").arg(_inlined);
	if (_pcLow)                 ret += QString("  pcLow:         %1\n").arg(_pcLow);
	if (_pcHigh)                ret += QString("  pcHigh:        %1\n").arg(_pcHigh);
	if (!_srcDir.isEmpty())     ret += QString("  srcDir:        %1\n").arg(_srcDir);
	if (_srcFileId != 0)        ret += QString("  srcFile:       0x%1\n").arg(_srcFileId, 0, 16);
	if (_srcLine >= 0)          ret += QString("  srcLine:       %1\n").arg(_srcLine);
	if (_fileIndex >= 0 && _fileIndex < symFiles.size())
		ret +=                         QString("  symFile:       %1\n").arg(symFiles[_fileIndex]);
	if (_sibling >= 0)          ret += QString("  sibling:       %1\n").arg(_sibling);
	if (!_enumValues.isEmpty()) {
	  ret +=                           QString("  enumValues:    ");
	  QList<qint32> keys = _enumValues.uniqueKeys();
	  qSort(keys);
	  bool first = true;
	  for (int i = 0; i < keys.size(); ++i) {
		  for (EnumHash::const_iterator it = _enumValues.find(keys[i]);
			   it != _enumValues.end() && it.key() == keys[i]; ++it) {
			  if (first)
				  first = false;
			  else
				  ret += ", ";
			  ret += it.value();
		  }
	  }
	  ret += "\n";
	}
    if (!_members.isEmpty()) {
      ret +=                           QString("  members:       ");
      for (int i = 0; i < _members.size(); i++) {
        ret += _members[i]->name();
        if (i + 1 < _members.size())
            ret += ", ";
      }
      ret += "\n";
    }

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
