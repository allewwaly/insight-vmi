#include "osfilter.h"
#include "filterexception.h"

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

static QHash<QString, QString> filters;

const QHash<QString, QString>& OsFilter::supportedFilters()
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
    : _osTypes(osIgnore), _architectures(arIgnore)
{
}


void OsFilter::clear()
{
    _osTypes = osIgnore;
    _architectures = arIgnore;
    _minVer.clear();
    _maxVer.clear();
}


void OsFilter::parseOption(const QString &key, const QString &val)
{
    QString k = key.toLower(), v = val.trimmed().toLower();

    if (k == str::architecture) {
        _architectures = arIgnore;
        // Process comma-separated list
        QStringList list = v.split(QChar(','), QString::SkipEmptyParts);
        for (int i = 0; i < list.size(); ++i) {
            QString a = list[i].trimmed();
            if (a == str::arX86)
                _architectures |= arX86;
            else if (a == str::arX86PAE)
                _architectures |= arX86PAE;
            else if (a == str::arAMD64)
                _architectures |= arAMD64;
            else
                filterError(QString("Unknown architecture: %1").arg(a));
        }
    }
    else if (k == str::os) {
        _osTypes = arIgnore;
        // Process comma-separated list
        QStringList list = v.split(QChar(','), QString::SkipEmptyParts);
        for (int i = 0; i < list.size(); ++i) {
            QString os = list[i].trimmed();
            if (os == str::osLinux)
                _osTypes |= osLinux;
            else if (os == str::osWindows)
                _osTypes |= osWindows;
            else
                filterError(QString("Unknown operating system: %1").arg(os));
        }
    }
    else if (k == str::minver) {
        _minVer = v.split(QRegExp("[-,.]"));
    }
    else if (k == str::maxver) {
        _maxVer = v.split(QRegExp("[-,.]"));
    }
    else
        filterError(QString("Unknown property: %1").arg(key));
}


bool OsFilter::match(const OsFilter &other) const
{
    // Compare architecture if set on both sides
    if (_architectures != arIgnore && other._architectures != arIgnore &&
        !(_architectures & other._architectures))
        return false;

    // Compare OS type if set on both sides
    if (_osTypes != osIgnore && other._osTypes != osIgnore &&
        !(_osTypes & other._osTypes))
        return false;

    // Compare min. version if set on both sides
    if (!_minVer.isEmpty() && !other._minVer.isEmpty() &&
        versionCmp(_minVer, other._minVer) > 0)
        return false;

    // Compare max. version if set on both sides
    if (!_maxVer.isEmpty() && !other._maxVer.isEmpty() &&
        versionCmp(_maxVer, other._maxVer) < 0)
        return false;

    return true;
}


int OsFilter::versionCmp(const QStringList &a, const QStringList &b)
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
