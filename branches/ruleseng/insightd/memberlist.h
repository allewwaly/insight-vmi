#ifndef MEMBERLIST_H
#define MEMBERLIST_H

#include <QList>

class StructuredMember;

/// A list of constant struct/union member objects
typedef QList<StructuredMember*> MemberList;

/// A list of constant struct/union member objects
typedef QList<const StructuredMember*> ConstMemberList;

#endif // MEMBERLIST_H
