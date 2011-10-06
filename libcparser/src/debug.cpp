#include <debug.h>

#if (DEBUG == 1)

std::ostream& operator<<(std::ostream& out, const QString &s)
{
	return out << qPrintable(s);
}

#endif
