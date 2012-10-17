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


namespace ProjectInfo
{
const char* projectName     = "InSight";
const char* homePage        = "https://code.google.com/p/insight-vmi/";
const char* bugTracker      = "https://code.google.com/p/insight-vmi/issues/list";
const char* devMailingList  = "insight-vmi-dev@googlegroups.com";
const char* userMailingList = "insight-vmi-discuss@googlegroups.com";
}


std::ostream& operator<<(std::ostream& out, const QString &s)
{
    return out << qPrintable(s);
}
