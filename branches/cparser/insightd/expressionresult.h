#ifndef ASTEXPRESSIONRESULT_H
#define ASTEXPRESSIONRESULT_H

#include <QString>
//#include <QDataStream>
#include "kernelsymbolstream.h"

/**
 The type of an epxression result, which may be a bit-wise combination of the
 following enumeration values.
 */
enum ExpressionResultType {
    erNotSet    = 0,         ///< Result is not set
    erConstant  = (1 << 0),  ///< Expression is compile-time constant
    erGlobalVar = (1 << 1),  ///< Expression involves global variable
    erLocalVar  = (1 << 2),  ///< Expression involves local variable
    erParameter = (1 << 3),  ///< Expression involves function parameters
    erRuntime   = (1 << 4),  ///< Expression involves run-time dependencies
//    erInvalid   = (1 << 5),  ///< Expression result cannot be determined
    erUndefined = (1 << 5)
};

/// The size and type of an ExpressionResult
enum ExpressionResultSize {
    esUndefined = 0,                  ///< size/type is undefined
    es8Bit      = (1 << 1),           ///< size is 8 bit
    es16Bit     = (1 << 2),           ///< size is 16 bit
    es32Bit     = (1 << 3),           ///< size is 32 bit
    es64Bit     = (1 << 4),           ///< size is 64 bit
    esUnsigned  = (1 << 5),           ///< type is unsigned
    esFloat     = (1 << 6),           ///< type is float
    esDouble    = (1 << 7),           ///< type is double
    esInt8      = es8Bit,             ///< result is 8 bit signed integer
    esUInt8     = es8Bit|esUnsigned,  ///< result is 8 bit unsigned integer
    esInt16     = es16Bit,            ///< result is 16 bit signed integer
    esUInt16    = es16Bit|esUnsigned, ///< result is 16 bit unsigned integer
    esInt32     = es32Bit,            ///< result is 32 bit signed integer
    esUInt32    = es32Bit|esUnsigned, ///< result is 32 bit unsigned integer
    esInt64     = es64Bit,            ///< result is 64 bit signed integer
    esUInt64    = es64Bit|esUnsigned, ///< result is 64 bit signed integer
    esInteger   = es8Bit|es16Bit|es32Bit|es64Bit, ///< result is an integer of any size
    esReal      = esFloat|esDouble    ///< result is a real value of any size
};


/// The result of an expression
struct ExpressionResult
{
    /// Constructor
    ExpressionResult() : resultType(erUndefined), size(esInt32) { this->result.i64 = 0; }
    explicit ExpressionResult(int resultType)
        : resultType(resultType), size(esInt32) { this->result.i64 = 0; }
    explicit ExpressionResult(int resultType, ExpressionResultSize size, quint64 result)
        : resultType(resultType), size(size) { this->result.ui64 = result; }
    explicit ExpressionResult(int resultType, ExpressionResultSize size, float result)
        : resultType(resultType), size(size) { this->result.f = result; }
    explicit ExpressionResult(int resultType, ExpressionResultSize size, double result)
        : resultType(resultType), size(size) { this->result.d = result; }

    /// ORed combination of ExpressionResultType values
    int resultType;

    /// size of result of expression. \sa ExpressionResultSize
    ExpressionResultSize size;

    /// Expression result, if valid
    union Result {
        quint64 ui64;
        qint64 i64;
        quint32 ui32;
        qint32 i32;
        quint16 ui16;
        qint16 i16;
        quint8 ui8;
        qint8 i8;
        float f;
        double d;
    } result;

    qint64 value(ExpressionResultSize target = esUndefined) const;

    quint64 uvalue(ExpressionResultSize target = esUndefined) const;

    float fvalue() const;

    double dvalue() const;

    QString toString() const;
};

/**
 * Operator for native usage of the ExpressionResult struct for streams
 * @param in data stream to read from
 * @param type object to store the serialized data to
 * @return the data stream \a in
 */
KernelSymbolStream& operator>>(KernelSymbolStream& in, ExpressionResult& result);

/**
 * Operator for native usage of the ExpressionResult struct for streams
 * @param out data stream to write the serialized data to
 * @param type object to serialize
 * @return the data stream \a out
 */
KernelSymbolStream& operator<<(KernelSymbolStream& out, const ExpressionResult& result);

#endif // ASTEXPRESSIONRESULT_H
