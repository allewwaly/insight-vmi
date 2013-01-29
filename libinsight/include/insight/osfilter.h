#ifndef OSFILTER_H
#define OSFILTER_H

#include <QString>
#include <QStringList>
#include <QHash>
#include "keyvaluestore.h"

namespace xml
{
extern const char* architecture;
extern const char* os;
extern const char* minver;
extern const char* maxver;

extern const char* arX86;
extern const char* arX86PAE;
extern const char* arAMD64;

extern const char* osLinux;
extern const char* osWindows;
}

struct MemSpecs;
class ColorPalette;

/**
 * This class describes the family, architecture and version of an operating
 * system.
 */
class OsSpecs
{
public:
    /// Specifies the operating system family
    enum OsFamily {
        ofIgnore  = 0,          ///< OS family is ignored
        ofLinux   = (1 << 0),   ///< OS family is Linux
        ofWindows = (1 << 1)    ///< OS family is Windows
    };

    /// Specifies the hardware architecture
    enum Architecture {
        arIgnore  = 0,         ///< architecture is ignored
        arX86     = (1 << 0),  ///< architecture is x86 (IA-32) without PAE
        arX86PAE  = (1 << 1),  ///< architecture is x86 with PAE enabled
        arAMD64   = (1 << 2)   ///< architecture is AMD64/Intel 64
    };

    /**
     * Constructor
     */
    OsSpecs() : _osFamily(ofIgnore), _architecture(arIgnore) {}

    /**
     * Constructor that initializes the OsSpecs object with values from
     * \a mspecs.
     * @param mspecs memory specifications to initialize from
     */
    OsSpecs(const MemSpecs* mspecs);

    /**
     * Resets all options to default values, i.e., "ignored".
     */
    void clear();

    /**
     * Initializes the OsSpecs object with values from \a mspecs.
     * @param mspecs memory specifications to initialize from
     */
    void setFrom(const MemSpecs* mspecs);

    /**
     * Returns the operating system families.
     * \sa setOsFamily(), OsFamily
     */
    inline OsFamily osFamily() const { return _osFamily; }

    /**
     * Sets the operating system family.
     * @param family OS family type
     * \sa osFamily(), OsFamily
     */
    inline void setOsFamily(OsFamily family) { _osFamily = family; }

    /**
     * Returns the architecture of this operating system.
     * \sa setArchitecture(), Architecture
     */
    inline Architecture architecture() const { return _architecture; }

    /**
     * Sets the architecture of this operating system.
     * @param arch logically ORed Architecture values
     * \sa architecture(), Architecture
     */
    inline void setArchitecture(Architecture arch) { _architecture = arch; }

    /**
     * Returns the version of this operating system.
     * \sa setVersion()
     */
    inline const QStringList& version() const { return _version; }

    /**
     * Sets the version of this operating system.
     * @param ver version
     * \sa version(), parseVersion()
     */
    inline void setVersion(const QString& ver) { _version = parseVersion(ver); }

    /**
     * Sets the version of this operating system.
     * @param ver version
     * \sa version(), parseVersion()
     */
    inline void setVersion(const QStringList& ver) { _version = ver; }

    /**
     * Splits up a version string like "x.y-z" into its components x, y, z.
     * @param version version string
     * @return components of version string
     */
    static QStringList parseVersion(const QString& version);

private:
    OsFamily _osFamily;
    Architecture _architecture;
    QStringList _version;
};

/**
 * This class provides a filter for the type of operating system.
 */
class OsFilter
{
public:
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
     * @param specs filter to match against
     * @return \c true if \a other is matched by this filter, \a false otherwise
     */
    inline bool match(const OsSpecs& specs) const { return match(&specs); }

    /**
     * Matches this filter against \a other.
     * @param specs filter to match against
     * @return \c true if \a other is matched by this filter, \a false otherwise
     */
    bool match(const OsSpecs* specs) const;

    /**
     * Returns the operating system families to match.
     * \sa setOsFamilies(), OsType
     */
    inline int osFamilies() const { return _osFamilies; }

    /**
     * Sets the operating system families to match.
     * @param families logically ORed OsSpecs::OsFamily's values
     * \sa osFamilies(), OsSpecs::OsFamily
     */
    inline void setOsFamilies(int families) { _osFamilies = families; }

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
     * \sa minVersion(), maxVersion(), OsSpecs::parseVersion()
     */
    inline void setMinVersion(const QStringList& ver) { _minVer = ver; }

    /**
     * Returns the maximum version to match.
     * \sa setMaxVersion(), minVersion()
     */
    inline const QStringList& maxVersion() const { return _maxVer; }

    /**
     * Sets the maximum version to match. Each version part is given as a string
     * in the list.
     * @param ver version
     * \sa maxVersion(), minVersion(), OsSpecs::parseVersion()
     */
    inline void setMaxVersion(const QStringList& ver) { _maxVer = ver; }

    /**
     * Returns a string representation of this filter.
     * @param col color palette to use for colorizing the output
     */
    QString toString(const ColorPalette *col = 0) const;

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
    int _osFamilies;
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
