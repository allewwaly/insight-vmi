#include "osfilter.h"
#include "filterexception.h"
#include <bitop.h>
#include "memspecs.h"

namespace str
{
const char* architecture = "architecture";
const char* os           = "os";
const char* minver       = "minversion";
const char* maxver       = "maxversion";

const char* arX86     = "x86";
const char* arX86PAE  = "x86_pae";
const char* arAMD64   = "amd64";

const char* osLinux   = "linux";
const char* osWindows = "windows";
}


OsSpecs::OsSpecs(const MemSpecs *mspecs)
    : _osFamily(ofIgnore), _architecture(arIgnore)
{
    setFrom(mspecs);
}


void OsSpecs::clear()
{
    _architecture = arIgnore;
    _osFamily = ofIgnore;
    _version.clear();
}


void OsSpecs::setFrom(const MemSpecs *mspecs)
{
    clear();
    if (mspecs) {
        // OS family
        if (mspecs->version.sysname.toLower() == "linux")
            setOsFamily(ofLinux);
        // Version
        if (!mspecs->version.release.isEmpty())
            setVersion(mspecs->version.release);
        // Architecture
        switch (mspecs->arch) {
        case MemSpecs::ar_i386:     setArchitecture(arX86);    break;
        case MemSpecs::ar_i386_pae: setArchitecture(arX86PAE); break;
        case MemSpecs::ar_x86_64:   setArchitecture(arAMD64);  break;
        default: break;
        }
    }
}


QStringList OsSpecs::parseVersion(const QString &version)
{
    return version.split(QRegExp("[-,.]"));
}



static KeyValueStore filters;

const KeyValueStore& OsFilter::supportedFilters()
{
    if (filters.isEmpty()) {
        filters[str::architecture] = "comma-separated list of hardware "
                "architectures:" +
                QString(str::arX86) + ", " +
                QString(str::arX86PAE) + ", " +
                QString(str::arAMD64);
        filters[str::os] = "comma-separated list of operating system types: " +
                QString(str::osLinux) + ", " + QString(str::osWindows);
        filters[str::minver] = "minimum operating system version";
        filters[str::maxver] = "maximum operating system version";
    }

    return filters;
}


OsFilter::OsFilter()
    : _osFamilies(OsSpecs::ofIgnore), _architectures(OsSpecs::arIgnore)
{
}


void OsFilter::clear()
{
    _osFamilies = OsSpecs::ofIgnore;
    _architectures = OsSpecs::arIgnore;
    _minVer.clear();
    _maxVer.clear();
}


void OsFilter::parseOption(const QString &key, const QString &val)
{
    QString k = key.toLower(), v = val.trimmed().toLower();

    if (k == str::architecture) {
        _architectures = OsSpecs::arIgnore;
        // Process comma-separated list
        QStringList list = v.split(QChar(','), QString::SkipEmptyParts);
        for (int i = 0; i < list.size(); ++i) {
            QString a = list[i].trimmed();
            if (a == str::arX86)
                _architectures |= OsSpecs::arX86;
            else if (a == str::arX86PAE)
                _architectures |= OsSpecs::arX86PAE;
            else if (a == str::arAMD64)
                _architectures |= OsSpecs::arAMD64;
            else
                filterError(QString("Unknown architecture: %1").arg(a));
        }
    }
    else if (k == str::os) {
        _osFamilies = OsSpecs::arIgnore;
        // Process comma-separated list
        QStringList list = v.split(QChar(','), QString::SkipEmptyParts);
        for (int i = 0; i < list.size(); ++i) {
            QString os = list[i].trimmed();
            if (os == str::osLinux)
                _osFamilies |= OsSpecs::ofLinux;
            else if (os == str::osWindows)
                _osFamilies |= OsSpecs::ofWindows;
            else
                filterError(QString("Unknown operating system: %1").arg(os));
        }
    }
    else if (k == str::minver) {
        _minVer = OsSpecs::parseVersion(v);
    }
    else if (k == str::maxver) {
        _maxVer = OsSpecs::parseVersion(v);
    }
    else
        filterError(QString("Unknown property: %1").arg(key));
}


bool OsFilter::match(const OsSpecs *specs) const
{
    // If nothing is specified, every specs match the filer
    if (!specs)
        return true;

    // Compare architecture if set on both sides
    if (_architectures != OsSpecs::arIgnore &&
        specs->architecture() != OsSpecs::arIgnore &&
        !(_architectures & specs->architecture()))
        return false;

    // Compare OS type if set on both sides
    if (_osFamilies != OsSpecs::ofIgnore &&
        specs->osFamily() != OsSpecs::ofIgnore &&
        !(_osFamilies & specs->osFamily()))
        return false;

    // Compare min. version if set on both sides
    if (!_minVer.isEmpty() && !specs->version().isEmpty() &&
        compareVersions(_minVer, specs->version()) > 0)
        return false;

    // Compare max. version if set on both sides
    if (!_maxVer.isEmpty() && !specs->version().isEmpty() &&
        compareVersions(_maxVer, specs->version()) < 0)
        return false;

    return true;
}


QString OsFilter::toString() const
{
#define commaIfNotFirst(str, first) suffixIfNotFirst(str, first, ", ")
#define suffixIfNotFirst(str, first, suffix) \
    do { \
        if (first) \
            first = false; \
        else \
            str += (suffix); \
    } while (0)

    QString s;
    if (_osFamilies) {
        s += "OS type: ";
        bool first = true;
        if (_osFamilies & OsSpecs::ofLinux) {
            commaIfNotFirst(s, first);
            s += str::osLinux;
        }
        if (_osFamilies & OsSpecs::ofLinux) {
            commaIfNotFirst(s, first);
            s += str::osWindows;
        }
        if (first)
            s += "(none)";
        s += "\n";
    }
    if (_architectures) {
        s += "Architecture: ";
        bool first = true;
        if (_architectures & OsSpecs::arX86) {
            commaIfNotFirst(s, first);
            s += str::arX86;
        }
        if (_architectures & OsSpecs::arX86PAE) {
            commaIfNotFirst(s, first);
            s += str::arX86PAE;
        }
        if (_architectures & OsSpecs::arAMD64) {
            commaIfNotFirst(s, first);
            s += str::arAMD64;
        }
        if (first)
            s += "(none)";
        s += "\n";
    }
    if (!_minVer.isEmpty())
        s += "Min. version: " + _minVer.join(".") + "\n";
    if (!_maxVer.isEmpty())
        s += "Max. version: " + _maxVer.join(".") + "\n";
    return s;
}


bool OsFilter::operator ==(const OsFilter &osf) const
{
    return _osFamilies == osf._osFamilies &&
            _architectures == osf._architectures &&
            _minVer == osf._minVer &&
            _maxVer == osf._maxVer;
}


int OsFilter::compareVersions(const QStringList &a, const QStringList &b)
{
    int ia, ib;
    bool a_ok, b_ok;
    for (int i = 0; i < a.size() && i < b.size(); ++i) {
        // Try a numerical comparison first
        ia = a[i].toUInt(&a_ok);
        ib = b[i].toUInt(&b_ok);
        if (a_ok && b_ok) {
            if (ia < ib)
                return -1;
            else if (ia > ib)
                return 1;
            else if (i+1 == a.size() && a.size() == b.size())
                return 0;
        }
        else {
            // Do a string comparison
            ia = QString::compare(a[i], b[i], Qt::CaseSensitive);
            if (ia)
                return ia;
        }
    }

    // Still no decision, so the more longer list is larger
    if (a.size() < b.size())
        return -1;
    else if (a.size() > b.size())
        return 1;
    else
        return 0;
}


uint qHash(const OsFilter& osf)
{
    uint ret, rot = 7;
    ret = osf.architectures();
    ret ^= rotl32(osf.osFamilies(), rot);
    rot = (rot + 7) % 32;
    ret ^= rotl32(qHash(osf.minVersion().join(".")), rot);
    rot = (rot + 7) % 32;
    ret ^= rotl32(qHash(osf.maxVersion().join(".")), rot);
    return ret;
}

