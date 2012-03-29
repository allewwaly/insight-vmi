#ifndef COLORPALETTE_H
#define COLORPALETTE_H

#include <QString>
#include <QList>
#include <QPair>


enum ColorType {
    ctReset = 0,
    ctBold,
    ctUnderline,
    ctInverse,
    ctPrompt,
    ctVariable,
    ctType,
    ctBuiltinType,
    ctRealType,
    ctMember,
    ctAddress,
    ctOffset,
    ctTypeId,
    ctKeyword,
    ctErrorLight,
    ctError,
    ctSrcFile,
    ctNoName,
    ctColHead,
    ctFuncParams,
    ctNumber,
    ctString,
    COLOR_TYPE_SIZE
};

typedef QPair<QString, const char*> NamePart;
typedef QList<NamePart> NamePartList;

class BaseType;

class ColorPalette
{
public:
    ColorPalette();

    bool allowColor() const;

    void setAllowColor(bool value);

    const char* color(ColorType ct) const;

    QString prettyNameInColor(const BaseType* t, int minLen, int maxLen = 0) const;

private:
    QString namePartsToString(const NamePartList& parts, int minLen = 0, int maxLen = 0) const;
    NamePartList prettyNameInColor(const BaseType* t) const;

    bool _allowColor;
};

#endif // COLORPALETTE_H
