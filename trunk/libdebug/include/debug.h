#ifndef DEBUG_H
#define DEBUG_H

// Enable/disable debug output globally
#define DEBUG 1

#ifdef __cplusplus

#	include <QString>
#	include <iostream>

/**
 * Operator that writes a QString to a std::ostream
 * @param out output stream to write into
 * @param s the QString to be written
 * @return the output stream with \a s written to it
 */
std::ostream& operator<<(std::ostream& out, const QString &s);

namespace VersionInfo
{
extern const char* release;
extern const char* svnRevision;
extern const char* buildDate;
extern const char* architecture;
}

namespace ProjectInfo
{
extern const char* projectName;
extern const char* homePage;
extern const char* bugTracker;
extern const char* devMailingList;
extern const char* userMailingList;
}

#endif /* __cplusplus */


#if (DEBUG == 1) && defined(__cplusplus)

// Enable debug output for symbol parsing
// (mostly in insightd/kernelsymbolparser.cpp)
#undef DEBUG_SYM_PARSING
//#define DEBUG_SYM_PARSING 1

// Activate for detailed node evaluation tracking
#undef DEBUG_NODE_EVAL
//#define DEBUG_NODE_EVAL 1

// Enable debug output for points-to analysis
// (mostly in libcparser/asttypeevaluator.cpp)
#undef DEBUG_POINTS_TO
//#define DEBUG_POINTS_TO 1

// Enable debug output for used-as analysis
// (mostly in libcparser/asttypeevaluator.cpp)
#undef DEBUG_USED_AS
//#define DEBUG_USED_AS 1

// Enable debug output for applying used-as relations to the types and symbols
// (mostly in insightd/symfactory.cpp)
#undef DEBUG_APPLY_USED_AS
//#define DEBUG_APPLY_USED_AS 1

// Enable debug output for type merging after parsing the kernel source code
// (mostly in insightd/symfactory.cpp)
#undef DEBUG_MERGE_TYPES_AFTER_PARSING
//#define DEBUG_MERGE_TYPES_AFTER_PARSING 1

#if !defined(_WIN32)
#define DEBUG_COLOR_DIM "\033[" "0" ";" "90" "m"
#define DEBUG_COLOR_ERR "\033[" "0" ";" "91" "m"
#define DEBUG_COLOR_RST "\033[" "0" "m"
#else
#define DEBUG_COLOR_DIM ""
#define DEBUG_COLOR_ERR ""
#define DEBUG_COLOR_RST ""
#endif

// #	include <QTime>
#	include <iomanip>
#	include <sstream>

#	ifndef assert
#		define assert(x) if ( !(x) ) \
					std::cerr << DEBUG_COLOR_DIM "(" __FILE__ ":" << __LINE__ << ")"\
								 DEBUG_COLOR_ERR " Assertion failed: " << #x << DEBUG_COLOR_RST << std::endl
#	endif

#	define debugerr(x) std::cerr << std::dec << DEBUG_COLOR_DIM "(" __FILE__ ":" << __LINE__ << ") " DEBUG_COLOR_ERR \
								<< x << std::endl << DEBUG_COLOR_RST << std::flush
#	define debugmsg(x) std::cout << std::dec << DEBUG_COLOR_DIM "(" __FILE__ ":" << __LINE__ << ") " DEBUG_COLOR_RST \
								<< x << std::endl << std::flush
/*#	define debugmsg(x) std::cout << std::dec << "(" << QTime::currentTime().toString("mm:ss.zzz").toStdString() << " " <<  __FILE__ << ":" << __LINE__ << ") " \
								<< x << std::endl << std::flush*/
#	define debugenter() debugmsg("entering " << __PRETTY_FUNCTION__)
#	define debugleave() debugmsg("leaving  " << __PRETTY_FUNCTION__)
#else
#	ifndef assert
#		define assert(x) do { (x); } while (0)
#	endif
#	define debugmsg(x)
#	define debugerr(x)
#	define debugenter(x)
#	define debugleave(x)
#endif /*DEBUG*/

#endif /* DEBUG_H */

