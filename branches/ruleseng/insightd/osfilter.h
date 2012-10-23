#ifndef OSFILTER_H
#define OSFILTER_H

#include <QString>
#include <QStringList>
#include <QHash>

/**
 * This class provides a filter for the type of operating system.
 */
class OsFilter
{
public:
    /// Specifies the type of operating system this filter applies to.
    enum OsType {
        osIgnore  = 0,          ///< OS type is ignored
        osLinux   = (1 << 0),   ///< OS type is Linux
        osWindows = (1 << 1)    ///< OS type is Windows
    };

    /// Specifies the hardware architecture this filter applies to.
    enum Architecture {
        arIgnore  = 0,         ///< architecture is ignored
        arX86     = (1 << 0),  ///< architecture is x86 (IA-32) without PAE
        arX86PAE  = (1 << 1),  ///< architecture is x86 with PAE enabled
        arAMD64   = (1 << 2)   ///< architecture is AMD64/Intel 64
    };

    /**
     * Constructor
     */
    OsFilter();

    /**
     * Resets all filter options to default values, i.e., "ignored".
     */
    void clear();

    /**
     * Parses a filter option from the given strings.
     * @param key the filter option to enable
     * @param val the value for the filter to match
     * \throws FilterException in case of unknown filters or other errors.
     */
    void parseOption(const QString& key, const QString& val);

    /**
     * Matches this filter against \a other.
     * @param other filter to match against
     * @return \c true if \a other is matched by this filter, \a false otherwise
     */
    bool match(const OsFilter& other) const;

    /**
     * Returns an associative list of filter options this class supports. The
     * key is the expected filter name, the value a short description of its
     * usage.
     */
    static const QHash<QString, QString>& supportedFilters();

private:
    /**
     * Compares two lists containing version information.
     * @param a alpha-numeric version information
     * @param b alpha-numeric version information
     * @return -1, 0, or 1 if \a a is less than, equal to or greater than \a b
     */
    static int versionCmp(const QStringList& a, const QStringList& b);

    int _osTypes;
    int _architectures;
    QStringList _minVer;
    QStringList _maxVer;
};

#endif // OSFILTER_H
