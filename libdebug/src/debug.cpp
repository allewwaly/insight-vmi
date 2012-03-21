#include <debug.h>


namespace VersionInfo
{
const char* svnRevision =
#ifdef SVN_REVISION
        SVN_REVISION;
#else
        "exported";
#endif

const char* release =
#ifdef INSIGHT_RELEASE
        INSIGHT_RELEASE;
#else
        "unknown";
#endif

const char* buildDate =
#ifdef BUILD_DATE
        BUILD_DATE;
#else
        "unknown";
#endif

const char* architecture =
#ifdef __x86_64__
        "x86_64";
#else
        "i386";
#endif
}

std::ostream& operator<<(std::ostream& out, const QString &s)
{
    return out << qPrintable(s);
}
