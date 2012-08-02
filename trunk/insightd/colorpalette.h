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
    ctWarningLight,
    ctWarning,
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

/**
 * This class produces ANSI color codes and supports the color output for ANSI
 * terminals. The color palette (light or dark background) is controlled
 * through the global ProgramOptions.
 *
 * \sa ProgramOptions, Options, opColorLightBg, opColorDarkBg
 */
class ColorPalette
{
public:
    /**
     * Constructor
     */
    ColorPalette();

    /**
     * @return \c true of color mode is currently enabled, \c false otherwise
     */
    bool allowColor() const;

    /**
     * Controls if color output is enabled or not. The default is to enable
     * color mode.
     * @param value Set to \c true to enable color output, set to \c false
     * otherwise.
     */
    void setAllowColor(bool value);

    /**
     * Returns the ANSI color code to produce the color for type \a ct.
     * @param ct the desired color type
     * @return color code to produce that color in an ANSI terminal
     */
    const char* color(ColorType ct) const;

    /**
     * Formats the pretty name of a type including ANSI color codes to produce
     * some syntax highlighting.
     * @param t the type to pretty-print
     * @param minLen the desired minimum length of the output string (excluding
     *  color codes); the string will be padded with spaces to match \a minLen
     * @param maxLen the desired maximum length of the output string (excluding
     *  color codes); the string will be shortend with "..." to be at most
     *  \a maxLen characters long.
     * @return the pretty name of type \a t, including ANSI color codes, padded
     *  or shortend to the given \a minLen and \a maxLen.
     */
    QString prettyNameInColor(const BaseType* t, int minLen, int maxLen = 0) const;

private:
    QString namePartsToString(const NamePartList& parts, int minLen = 0, int maxLen = 0) const;
    NamePartList prettyNameInColor(const BaseType* t) const;

    bool _allowColor;
};

#endif // COLORPALETTE_H
