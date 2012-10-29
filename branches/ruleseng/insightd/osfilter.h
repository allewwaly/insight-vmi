#ifndef OSFILTER_H
#define OSFILTER_H

#include <QString>
#include <QStringList>
#include <QHash>
#include "keyvaluestore.h"

namespace str
{
extern const char* architecture;
extern const char* os;
extern const char* minver;
extern const char* maxver;
}

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
     * Returns the operating system types to match.
     * \sa setOsType(), OsType
     */
    inline int osTypes() const { return _osTypes; }

    /**
     * Sets the operating system types to match.
     * @param types logically ORed OsType values
     * \sa osTypes(), OsType
     */
    inline void setOsTypes(int types) { _osTypes = types; }

    /**
     * Returns the architectures to match.
     * \sa setArchitectures(), Architecture
     */
    inline int architectures() const { return _architectures; }

    /**
     * Sets the architectures to match.
     * @param arch logically ORed Architecture values
     * \sa architectures(), Architecture
     */
    inline void setArchitectures(int arch) { _architectures = arch; }

    /**
     * Returns the minimum version to match.
     * \sa setMinVersion(), maxVersion()
     */
    inline const QStringList& minVersion() const { return _minVer; }

    /**
     * Sets the minimum version to match. Each version part is given as a string
     * in the list.
     * @param ver version
     * \sa minVersion(), maxVersion()
     */
    inline void setMinVersion(const QStringList ver) { _minVer = ver; }

    /**
     * Returns the maximum version to match.
     * \sa setMaxVersion(), minVersion()
     */
    inline const QStringList& maxVersion() const { return _maxVer; }

    /**
     * Sets the maximum version to match. Each version part is given as a string
     * in the list.
     * @param ver version
     * \sa maxVersion(), minVersion()
     */
    inline void setMaxVersion(const QStringList ver) { _maxVer = ver; }

    /**
     * Returns a string representation of this filter.
     */
    QString toString() const;

    /**
     * Comparison operator
     * @param osf object to compare to
     * @return \c true if objects are equal, false otherwise
     */
    bool operator==(const OsFilter& osf) const;

    /**
     * Comparison operator
     * @param osf object to compare to
     * @return \c true if objects are not equal, false otherwise
     */
    inline bool operator!=(const OsFilter& osf) const { return !operator==(osf); }

    /**
     * Returns an associative list of filter options this class supports. The
     * key is the expected filter name, the value a short description of its
     * usage.
     */
    static const KeyValueStore& supportedFilters();

    /**
     * Compares two lists containing version information.
     * @param a alpha-numeric version information
     * @param b alpha-numeric version information
     * @return -1, 0, or 1 if \a a is less than, equal to or greater than \a b
     */
    static int compareVersions(const QStringList& a, const QStringList& b);

private:
    int _osTypes;
    int _architectures;
    QStringList _minVer;
    QStringList _maxVer;
};

/**
 * Returns a hash value for an OsFilter object
 * @param osf object to hash
 * @return hash value
 */
uint qHash(const OsFilter& osf);

#endif // OSFILTER_H
