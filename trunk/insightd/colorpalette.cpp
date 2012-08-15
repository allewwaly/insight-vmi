#include "colorpalette.h"
#include "programoptions.h"
#include "array.h"
#include "funcpointer.h"

// ANSI color codes
#define COLOR_BLACK    "30"
#define COLOR_RED      "31"
#define COLOR_GREEN    "32"
#define COLOR_YELLOW   "33"
#define COLOR_BLUE     "34"
#define COLOR_MAGENTA  "35"
#define COLOR_CYAN     "36"
#define COLOR_WHITE    "37"

#define COLOR_LBLACK   "90"
#define COLOR_LRED     "91"
#define COLOR_LGREEN   "92"
#define COLOR_LYELLOW  "93"
#define COLOR_LBLUE    "94"
#define COLOR_LMAGENTA "95"
#define COLOR_LCYAN    "96"
#define COLOR_LWHITE   "97"

#define S_NONE      "0"
#define S_BOLD      "1"
#define S_UNDERLINE "4"
#define S_INV       "7"

#define DEFSTYLE(s) "\033[" s "m"
#define DEFCOLOR(c, s) "\033[" s ";" c "m"

#define C_RST DEFSTYLE(S_NONE)
#define C_BLD DEFSTYLE(S_BOLD)
#define C_UDL DEFSTYLE(S_UNDERLINE)
#define C_INV DEFSTYLE(S_INV)

#define C_BLK   DEFCOLOR(COLOR_BLACK, S_NONE)
#define C_BLK_L DEFCOLOR(COLOR_LBLACK, S_NONE)
#define C_BLK_B DEFCOLOR(COLOR_BLACK, S_BOLD)
#define C_BLK_U DEFCOLOR(COLOR_BLACK, S_UNDERLINE)
#define C_BLK_I DEFCOLOR(COLOR_BLACK, S_INV)

#define C_RED   DEFCOLOR(COLOR_RED, S_NONE)
#define C_RED_L DEFCOLOR(COLOR_LRED, S_NONE)
#define C_RED_B DEFCOLOR(COLOR_RED, S_BOLD)
#define C_RED_U DEFCOLOR(COLOR_RED, S_UNDERLINE)
#define C_RED_I DEFCOLOR(COLOR_RED, S_INV)

#define C_GRN   DEFCOLOR(COLOR_GREEN, S_NONE)
#define C_GRN_L DEFCOLOR(COLOR_LGREEN, S_NONE)
#define C_GRN_B DEFCOLOR(COLOR_GREEN, S_BOLD)
#define C_GRN_U DEFCOLOR(COLOR_GREEN, S_UNDERLINE)
#define C_GRN_I DEFCOLOR(COLOR_GREEN, S_INV)

#define C_YLW   DEFCOLOR(COLOR_YELLOW, S_NONE)
#define C_YLW_L DEFCOLOR(COLOR_LYELLOW, S_NONE)
#define C_YLW_B DEFCOLOR(COLOR_YELLOW, S_BOLD)
#define C_YLW_U DEFCOLOR(COLOR_YELLOW, S_UNDERLINE)
#define C_YLW_I DEFCOLOR(COLOR_YELLOW, S_INV)

#define C_BLU   DEFCOLOR(COLOR_BLUE, S_NONE)
#define C_BLU_L DEFCOLOR(COLOR_LBLUE, S_NONE)
#define C_BLU_B DEFCOLOR(COLOR_BLUE, S_BOLD)
#define C_BLU_U DEFCOLOR(COLOR_BLUE, S_UNDERLINE)
#define C_BLU_I DEFCOLOR(COLOR_BLUE, S_INV)

#define C_MGT   DEFCOLOR(COLOR_MAGENTA, S_NONE)
#define C_MGT_L DEFCOLOR(COLOR_LMAGENTA, S_NONE)
#define C_MGT_B DEFCOLOR(COLOR_MAGENTA, S_BOLD)
#define C_MGT_U DEFCOLOR(COLOR_MAGENTA, S_UNDERLINE)
#define C_MGT_I DEFCOLOR(COLOR_MAGENTA, S_INV)

#define C_CYN   DEFCOLOR(COLOR_CYAN, S_NONE)
#define C_CYN_L DEFCOLOR(COLOR_LCYAN, S_NONE)
#define C_CYN_B DEFCOLOR(COLOR_CYAN, S_BOLD)
#define C_CYN_U DEFCOLOR(COLOR_CYAN, S_UNDERLINE)
#define C_CYN_I DEFCOLOR(COLOR_CYAN, S_INV)

#define C_WHT   DEFCOLOR(COLOR_WHITE, S_NONE)
#define C_WHT_L DEFCOLOR(COLOR_LWHITE, S_NONE)
#define C_WHT_B DEFCOLOR(COLOR_WHITE, S_BOLD)
#define C_WHT_U DEFCOLOR(COLOR_WHITE, S_UNDERLINE)
#define C_WHT_I DEFCOLOR(COLOR_WHITE, S_INV)


ColorPalette::ColorPalette()
    : _allowColor(true)
{
}


ColorMode ColorPalette::colorMode() const
{
    if (_allowColor) {
        if (programOptions.activeOptions() & opColorDarkBg)
            return cmDarkBg;
        else if (programOptions.activeOptions() & opColorLightBg)
            return cmLightBg;
    }
    return cmOff;
}


bool ColorPalette::allowColor() const
{
    return _allowColor;
}


void ColorPalette::setAllowColor(bool value)
{
    _allowColor = value;
}


const char *ColorPalette::color(ColorType ct) const
{

    static const char* colors_dark[COLOR_TYPE_SIZE] = {
        C_RST,   // ctReset
        C_BLD,   // ctBold
        C_UDL,   // ctUnderline
        C_INV,   // ctInverse

        C_GRN,   // ctPrompt
        C_YLW_B, // ctVariable
        C_CYN_B, // ctType
        C_BLU_B, // ctBuiltinType
        C_BLK_B, // ctRealType
        C_GRN_B, // ctMember
        C_RED_L, // ctAddress
        C_RED_L, // ctOffset
        C_YLW_L, // ctTypeId
        C_MGT_L, // ctKeyword
        C_RED_B, // ctErrorLight
        C_RED_L, // ctError
        C_YLW_B, // ctWarningLight
        C_YLW_L, // ctWarning
        C_RST,   // ctSrcFile
        C_BLK_B, // ctNoName
        C_BLK_B, // ctColHead
        C_RST,   // ctFuncParams
        C_CYN_L, // ctNumber
        C_GRN_L, // ctString
    };

    static const char* colors_light[COLOR_TYPE_SIZE] = {
        C_RST,   // ctReset
        C_BLD,   // ctBold
        C_UDL,   // ctUnderline
        C_INV,   // ctInverse

        C_BLU,   // ctPrompt
        C_RED_B, // ctVariable
        C_BLU_B, // ctType
        C_CYN,   // ctBuiltinType
        C_BLK_B, // ctRealType
        C_MGT_B, // ctMember
        C_RED,   // ctAddress
        C_RED,   // ctOffset
        C_GRN,   // ctTypeId
        C_MGT,   // ctKeyword
        C_RED_B, // ctErrorLight
        C_RED,   // ctError
        C_MGT_B, // ctWarningLight
        C_MGT,   // ctWarning
        C_RST,   // ctSrcFile
        C_BLK_B, // ctNoName
        C_WHT,   // ctColHead
        C_RST,   // ctFuncParams
        C_BLU,   // ctNumber
        C_GRN,   // ctString
    };

    static const char* empty = "";

    if ((ct < 0) || (ct >= COLOR_TYPE_SIZE))
        return empty;

    // Which color mode is enabled?
    switch (colorMode()) {
    case cmDarkBg:  return colors_dark[ct];
    case cmLightBg: return colors_light[ct];
    default:        return empty;
    }

    return empty;
}


QString ColorPalette::prettyNameInColor(const BaseType* t, int minLen, int maxLen) const
{
    if (!t)
        return QString();

    NamePartList parts = prettyNameInColor(t);
    return namePartsToString(parts, minLen, maxLen);
}


QString ColorPalette::namePartsToString(const NamePartList &parts, int minLen, int maxLen) const
{
    int len = 0;
    QString ret;
    int funcParams = 0;
    bool endsWithReset = false;

    for (int i = 0; i < parts.size(); ++i) {
        const QString& part = parts[i].first;
        const char* col = parts[i].second;
        if (part.isEmpty())
            continue;
        if (col)
            ret += col;
        else
            ret += color(ctReset);
        endsWithReset = (col == 0);

        if (maxLen > 1) {
            if (len + part.size() <= maxLen) {
                ret += part;
                len += part.size();
                if (part == ")(" || part == "(")
                    ++funcParams;
                else if (part == ")")
                    --funcParams;
            }
            // We need to shorten it
            else {
                int remLen = maxLen - len - ((funcParams > 0) ? 4 : 3);
                if (remLen < 0)
                    remLen = 0;
                ret += part.left(remLen);
                ret += color(ctReset);
                if (funcParams > 0)
                    ret += QString("%1").arg(")", maxLen - len - remLen, QChar('.'));
                else
                    ret += QString("%1").arg("", maxLen - len - remLen, QChar('.'));
                len = maxLen;
                break;
            }
        }
        else {
            ret += part;
            len += part.size();
        }
    }

    if (!endsWithReset)
        ret += color(ctReset);

    // Do we need to the fill the string?
    if (len < minLen)
        ret += QString("%1").arg("", minLen - len);

    return ret;
}


NamePartList ColorPalette::prettyNameInColor(const BaseType *t) const
{
    if (!t)
        return NamePartList();

    NamePartList parts;

    const RefBaseType* rbt = dynamic_cast<const RefBaseType*>(t);

    switch (t->type()) {
    case rtInt8:
    case rtUInt8:
    case rtBool8:
    case rtInt16:
    case rtUInt16:
    case rtBool16:
    case rtInt32:
    case rtUInt32:
    case rtBool32:
    case rtInt64:
    case rtUInt64:
    case rtBool64:
    case rtFloat:
    case rtDouble:
//        if (t->name().isEmpty())
            parts.append(NamePart(t->prettyName(), color(ctBuiltinType)));
//        else
//            parts.append(NamePart(t->name(), color(ctType)));
        break;

    case rtEnum:
        if (t->name().isEmpty())
            parts.append(NamePart("(none)", color(ctNoName)));
        else
            parts.append(NamePart(t->name(), color(ctType)));
        break;

    case rtStruct:
    case rtUnion:
        parts.append(NamePart(t->type() == rtStruct ? "struct " : "union ",
                              color(ctKeyword)));
        if (t->name().isEmpty())
            parts.append(NamePart("(anon.)", color(ctNoName)));
        else
            parts.append(NamePart(t->name(), color(ctType)));
        break;

    case rtTypedef:
        parts.append(NamePart(t->name(), color(ctType)));
        break;

    case rtConst:
        parts.append(NamePart("const", color(ctKeyword)));
        if (rbt->refTypeId() == 0)
            parts.append(NamePart(" void", color(ctKeyword)));
        else {
            parts.append(NamePart(" ", 0));
            parts += prettyNameInColor(rbt->refType());
        }
        break;

    case rtVolatile:
        parts.append(NamePart("volatile ", color(ctKeyword)));
        if (rbt->refTypeId() == 0)
            parts.append(NamePart("void", color(ctKeyword)));
        else
            parts += prettyNameInColor(rbt->refType());
        break;

    case rtPointer:
        if (rbt->refTypeId() == 0)
            parts.append(NamePart("void", color(ctKeyword)));
        else
            parts = prettyNameInColor(rbt->refType());
        parts.append(NamePart(" *", color(ctType)));
        break;

    case rtArray: {
        const Array* a = dynamic_cast<const Array*>(rbt);
        if (rbt->refTypeId() == 0)
            parts.append(NamePart("void", color(ctKeyword)));
        else
            parts = prettyNameInColor(rbt->refType());
        parts.append(NamePart("[", color(ctReset)));
        parts.append(NamePart(a->length() < 0 ?
                                  QString("?") : QString::number(a->length()),
                              0));
        parts.append(NamePart("]", 0));
        break;
    }

    case rtFunction:
    case rtFuncPointer: {
        const FuncPointer* fp = dynamic_cast<const FuncPointer*>(rbt);
        QString s;
        for (int i = 0; i < fp->params().size(); ++i) {
            if (i > 0)
                s += ", ";
            s += fp->params().at(i)->prettyName();
        }
        if (fp->refTypeId())
            parts = prettyNameInColor(fp->refType());
        else
            parts.append(NamePart("void", color(ctKeyword)));
        if (fp->type() & rtFuncPointer) {
            parts.append(NamePart(" (", 0));
            parts.append(NamePart(QString("*%1").arg(fp->name()), color(ctType)));
            parts.append(NamePart(")(", 0));
        }
        else {
            parts.append(NamePart(QString(" %1").arg(fp->name()), color(ctType)));
            parts.append(NamePart("(", 0));
        }
        if (!s.isEmpty())
            parts.append(NamePart(s, color(ctFuncParams)));
        parts.append(NamePart(")", 0));
        break;
    }

    default:
        break;

    }

    return parts;
}
