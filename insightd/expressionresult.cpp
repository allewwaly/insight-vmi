
#include "expressionresult.h"
#include "expressionevalexception.h"
#include <debug.h>

qint64 ExpressionResult::value(ExpressionResultSize target) const
{
    qint64 ret;

    // Is either the target or this value unsigned?
    if (target && ((target | size) & esUnsigned)) {
        // If the target is unsigned, return the unsigned  value instead.
        // If this value is unsigned, do not extend the sign!
        ret = uvalue((ExpressionResultSize)(target | esUnsigned));
    }
    // No target specified, or both target and this value are signed, so
    // extend this value's sign to 64 bit
    else {
        ret = (size & es64Bit) ?
                    result.i64 :
                    (size & es32Bit) ?
                        (qint64) result.i32 :
                        (size & es16Bit) ?
                            (qint64) result.i16 :
                            (qint64) result.i8;
    }
//        debugmsg(QString("Returning 0x%1").arg(ret, 16, 16, QChar('0')));
    return ret;
}


quint64 ExpressionResult::uvalue(ExpressionResultSize target) const
{
    quint64 ret;

    // Is either the target or this value unsigned?
    if (target && !(target & size & esUnsigned)) {
        // If the target is signed, return the signed value instead
        if (!(target & esUnsigned))
            ret = value(target);
        // If this is a signed value and the target is unsigned, extend the
        // sign up to target size before converting to unsigned
        else {
            switch (target & (es64Bit|es32Bit|es16Bit|es8Bit)) {
            case es64Bit:
                ret = (size & es64Bit) ?
                            result.i64 :
                            (size & es32Bit) ?
                                (quint64) result.i32 :
                                (size & es16Bit) ?
                                    (quint64) result.i16 :
                                    (quint64) result.i8;
                break;

            case es32Bit:
                ret = (size & (es64Bit|es32Bit)) ?
                            result.ui32 :
                            (size & es16Bit) ?
                                (quint32) result.i16 :
                                (quint32) result.i8;
                break;

            case es16Bit:
                ret = (size & (es64Bit|es32Bit|es16Bit)) ?
                            result.ui16 :
                            (quint16) result.i8;
                break;

            case es8Bit:
                ret = result.ui8;
                break;

            default:
                exprEvalError(QString("Invalid target size: %1").arg(target));
                break;
            }
        }
    }
    else {
        ret = (size & es64Bit) ?
                result.ui64 :
                (size & es32Bit) ?
                    (quint64) result.ui32 :
                    (size & es16Bit) ?
                        (quint64) result.ui16 :
                        (quint64) result.ui8;
    }
//        debugmsg(QString("Returning 0x%1").arg(ret, 16, 16, QChar('0')));
    return ret;
}


float ExpressionResult::fvalue() const
{
    // Convert any value to float
    if (size & esDouble)
        return result.d;
    else if (size & esFloat)
        return result.f;
    else if (size & esUnsigned)
        return uvalue();
    else
        return value();
}


double ExpressionResult::dvalue() const
{
    // Convert any value to double
    if (size & esDouble)
        return result.d;
    else if (size & esFloat)
        return result.f;
    else if (size & esUnsigned)
        return uvalue();
    else
        return value();
}


QString ExpressionResult::toString() const
{
    if (resultType == erUndefined)
        return "(undefined value)";
    else if (resultType & erRuntime)
        return "(runtime expression)";
    else if (resultType & erInvalid)
        return "(invalid value)";
    else if (resultType & erVoid)
        return "(void)";

    switch (size) {
    case esUndefined: return "(undefined value)";
    case esFloat:     return QString::number(result.f);
    case esDouble:    return QString::number(result.d);
    case esInt8:      return QString::number(result.i8);
    case esUInt8:     return QString::number(result.ui8);
    case esInt16:     return QString::number(result.i16);
    case esUInt16:    return QString::number(result.ui16);
    case esInt32:
        return (result.i32 > 65535 || result.i32 < -65535) ?
                    "0x" + QString::number(result.i32, 16) :
                    QString::number(result.i32);
    case esUInt32:
        return (result.ui32 > 65535) ?
                    "0x" + QString::number(result.ui32, 16) :
                    QString::number(result.ui32);
    case esInt64:
        return (result.i64 > 65535 || result.i64 < -65535) ?
                    "0x" + QString::number(result.i64, 16) :
                    QString::number(result.i64);
    case esUInt64:
        return (result.ui64 > 65535) ?
                    "0x" + QString::number(result.ui64, 16) :
                    QString::number(result.ui64);
    default:
        return (size & esUnsigned) ?
                    QString::number(uvalue()) :
                    QString::number(value());
    }
}
