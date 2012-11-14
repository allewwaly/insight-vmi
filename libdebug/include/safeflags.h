#ifndef SAFEFLAGS_H
#define SAFEFLAGS_H

#include <QFlags>

#ifdef NO_TYPESAFE_FLAGS

#define DECLARE_SAFE_FLAGS(Flags, Enum) typedef uint Flags;

#else

//typedef QFlags<Enum> Flags;
#define DECLARE_SAFE_FLAGS(Flags, Enum) Q_DECLARE_FLAGS(Flags, Enum)

#endif /* NO_TYPESAFE_FLAGS */


#endif // SAFEFLAGS_H
