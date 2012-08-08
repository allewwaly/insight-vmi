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


// Enabled the code for memory map building (requires the X window system)
//#define CONFIG_MEMORY_MAP 1


#if (DEBUG == 1) && defined(__cplusplus)

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



// #	include <QTime>
#	include <iomanip>
#	include <sstream>

#	ifndef assert
#		define assert(x) if ( !(x) ) \
					std::cerr << "(" << __FILE__ << ":" << __LINE__ << ") "\
							<< " Assertion failed: " << #x << std::endl
#	endif

#	define debugerr(x) std::cerr << std::dec << "(" << __FILE__ << ":" << __LINE__ << ") " \
								<< x << std::endl << std::flush
#	define debugmsg(x) std::cout << std::dec << "(" __FILE__ ":" << __LINE__ << ") " \
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

