#ifndef MEMBERLIST_H
#define MEMBERLIST_H

#include <QList>

class StructuredMember;
class BaseType;

/// A list of constant struct/union member objects
typedef QList<StructuredMember*> MemberList;

/// A list of constant struct/union member objects
typedef QList<const StructuredMember*> ConstMemberList;

/// Holds VariableTypeContainer structs
typedef QList<struct VariableTypeContainer> VariableTypeContainerList;

struct VariableTypeContainer
{
    VariableTypeContainer() : type(0), index(-1) {}
    VariableTypeContainer(const BaseType *type) :
                          type(type), index(-1) {}
    VariableTypeContainer(const BaseType *type, int index) :
                          type(type), index(index) {}

    const BaseType *type;
    int index;
};

#endif // MEMBERLIST_H
