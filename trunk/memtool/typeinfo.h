/*
 * typeinfo.h
 *
 *  Created on: 31.03.2010
 *      Author: chrschn
 */

#ifndef TYPEINFO_H_
#define TYPEINFO_H_

#include <QString>
#include <QHash>

enum HdrSymbolType {
	hsUnknownSymbol         = 0,         // Start with 0 for any unknown symbol
	hsArrayType             = (1 <<  0), // This represents first "real" symbol
	hsBaseType              = (1 <<  1),
	hsCompileUnit           = (1 <<  2),
	hsConstType             = (1 <<  3),
	hsEnumerationType       = (1 <<  4),
	hsEnumerator            = (1 <<  5),
	hsFormalParameter       = (1 <<  6),
	hsInlinedSubroutine     = (1 <<  7),
	hsLabel                 = (1 <<  8),
	hsLexicalBlock          = (1 <<  9),
	hsMember                = (1 << 10),
	hsPointerType           = (1 << 11),
	hsStructureType         = (1 << 12),
	hsSubprogram            = (1 << 13),
	hsSubrangeType          = (1 << 14),
	hsSubroutineType        = (1 << 15),
	hsTypedef               = (1 << 16),
	hsUnionType             = (1 << 17),
	hsUnspecifiedParameters = (1 << 18),
	hsVariable              = (1 << 19),
	hsVolatileType          = (1 << 20)
};

enum ParamSymbolType {
	psUnknownSymbol      = 0,         // Start with 0 for any unknown symbol
	psAbstractOrigin     = (1 <<  0), // This represents first "real" symbol
	psArtificial         = (1 <<  1),
	psBitOffset          = (1 <<  2),
	psBitSize            = (1 <<  3),
	psByteSize           = (1 <<  4),
	psCallFile           = (1 <<  5),
	psCallLine           = (1 <<  6),
	psCompDir            = (1 <<  7),
	psConstValue         = (1 <<  8),
	psDataMemberLocation = (1 <<  9),
	psDeclaration        = (1 << 10),
	psDeclFile           = (1 << 11),
	psDeclLine           = (1 << 12),
	psEncoding           = (1 << 13),
	psEntryPc            = (1 << 14),
	psExternal           = (1 << 15),
	psFrameBase          = (1 << 16),
	psHighPc             = (1 << 17),
	psInline             = (1 << 18),
	psLanguage           = (1 << 19),
	psLocation           = (1 << 20),
	psLowPc              = (1 << 21),
	psName               = (1 << 22),
	psProducer           = (1 << 23),
	psPrototyped         = (1 << 24),
	psRanges             = (1 << 25),
	psSibling            = (1 << 26),
	psStmtList           = (1 << 27),
	psType               = (1 << 28),
	psUpperBound         = (1 << 29)
};

/// The data encoding of a type
enum DataEncoding {
	eUndef     = 0,         // Start with 0 for any unknown symbol
	eSigned    = (1 << 0),  // This represents first "real" symbol
	eUnsigned  = (1 << 1),
	eFloat     = (1 << 2)
};


typedef QHash<QString, HdrSymbolType> HdrSymMap;
typedef QHash<QString, ParamSymbolType> ParamSymMap;
typedef QHash<QString, DataEncoding> DataEncMap;

typedef QHash<HdrSymbolType, QString> HdrSymRevMap;
typedef QHash<ParamSymbolType, QString> ParamSymRevMap;

HdrSymMap getHdrSymMap();
ParamSymMap getParamSymMap();
DataEncMap getDataEncMap();

/**
 * Returns an inverted value of a QHash, i.e., a hash with key and value
 * types swapped.
 * @param hash the QHash to be inverted
 * @return an inverted version of @a hash
 */
template<class T>
QHash<typename T::mapped_type, typename T::key_type> invertHash(T hash)
{
    QHash<typename T::mapped_type, typename T::key_type> ret;
    typename T::iterator it;
    for (it = hash.begin(); it != hash.end(); ++it)
        ret.insert(it.value(), it.key());
    return ret;
}


static const quint32 relevantHdr =
	hsArrayType |
	hsBaseType |
	hsCompileUnit |
	hsConstType |
	hsEnumerationType |
	hsEnumerator |
//	hsFormalParameter |
//	hsInlinedSubroutine |
//	hsLabel |
//	hsLexicalBlock |
	hsMember |
	hsPointerType |
	hsStructureType |
//	hsSubprogram |
	hsSubrangeType |
	hsSubroutineType |
	hsTypedef |
	hsUnionType |
	hsUnspecifiedParameters |
	hsVariable |
	hsVolatileType;

static const quint32 relevantParam =
//	psAbstractOrigin |
//	psArtificial |
	psBitOffset |
	psBitSize |
	psByteSize |
//	psCallFile |
//	psCallLine |
	psCompDir |
	psConstValue |
	psDataMemberLocation |
	psDeclaration |
	psDeclFile |
	psDeclLine |
	psEncoding |
//	psEntryPc |
//	psExternal |
//	psFrameBase |
//	psHighPc |
//	psInline |
//	psLanguage |
	psLocation |
//	psLowPc |
	psName |
//	psProducer |
//	psPrototyped |
//	psRanges |
//	psSibling |
//	psStmtList |
	psType |
	psUpperBound;


class TypeInfo
{
public:
	/**
	 * Constructor
	 */
	TypeInfo();

	/**
	 * Resets all data to their default (empty) values
	 */
	void clear();

    const QString& name() const;
    void setName(QString name);

    const QString& srcDir() const;
    void setSrcDir(QString name);

    int srcFileId() const;
    void setSrcFileId(int id);

    int srcLine() const;
    void setSrcLine(int lineno);

    int id() const;
    void setId(int id);

    int refTypeId() const;
    void setRefTypeId(int refTypeID);

    quint32 byteSize() const;
    void setByteSize(quint32 byteSize);

    qint32 location() const;
    void setLocation(qint32 location);

    qint32 dataMemberLoc() const;
    void setDataMemberLoc(qint32 location);

    HdrSymbolType symType() const;
    void setSymType(HdrSymbolType symType);

    DataEncoding enc() const;
    void setEnc(DataEncoding enc);

    qint32 upperBound() const;
    void setUpperBound(qint32 bount);

    QString dump() const;

private:
	QString _name;           ///< holds the name of this symbol
	QString _srcDir;         ///< holds the directory of the compile unit
	int _srcFileId;          ///< holds the ID of the source file
	int _srcLine;            ///< holds the line number within the source file
	int _id;                 ///< holds the ID of this symbol
	int _refTypeId;          ///< holds the ID of the referenced symbol
	quint32 _byteSize;       ///< holds the size in byte of this symbol
	qint32 _location;        ///< holds the absolute offset offset of this symbol
	qint32 _dataMemberLoc;   ///< holds the offset relative offset of this symbol
	qint32 _upperBound;      ///< holds the upper bound for an integer type symbol
	HdrSymbolType _symType;  ///< holds the type of this symbol
	DataEncoding _enc;       ///< holds the data encoding of this symbol
};


#endif /* TYPEINFO_H_ */
