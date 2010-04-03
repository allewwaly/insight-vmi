#ifndef DEBUG_H
#define DEBUG_H

//#ifndef DEBUG
#define DEBUG 1
//#endif

#if (DEBUG == 1)

#	include <QString>
// #	include <QTime>
#	include <iostream>
#	include <iomanip>
#	include <sstream>

/**
 * Operator that writes a QString to a std::ostream
 * @param out output stream to write into
 * @param s the QString to be written
 * @return the output stream with \a s written to it
 */
std::ostream& operator<<(std::ostream& out, const QString &s);

#	ifndef assert
#		define assert(x) if ( !(x) ) \
					std::cerr << "(" << __FILE__ << ":" << __LINE__ << ") "\
							<< " Assertion failed: " << #x << std::endl
#	endif

#	define debugerr(x) std::cerr << "(" << __FILE__ << ":" << __LINE__ << ") " \
								<< x << std::endl << std::flush
#	define debugmsg(x) std::cout << "(" __FILE__ ":" << __LINE__ << ") " \
								<< x << std::endl << std::flush
/*#	define debugmsg(x) std::cout << "(" << QTime::currentTime().toString("mm:ss.zzz").toStdString() << " " <<  __FILE__ << ":" << __LINE__ << ") " \
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

