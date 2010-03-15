#ifndef BASETYPE_H
#define BASETYPE_H

#include <sys/types.h>
#include <QtGlobal>
#include <QString>
//#include "instance.h"


// template <typename T>
class BaseType
{
public:
    enum RealType {
        rtChar,
        rtUChar,
        rtInt,
        rtUInt,
        rtLong,
        rtULong,
        rtFloat,
        rtDouble,
        rtPointer,
        rtStruct,
        rtUnion
    };

    /**
     * Constructor
     * @param name name of this type, e.g. "int"
     * @param id ID of this type, as given by objdump output
     * @param size size of this type in bytes
     */
    BaseType(const QString& name, const quint32 id, const quint32 size);

//    /**
//     * Destructor
//     */
//    ~BaseType();

    /**
     * @return the actual type of that polimorphic instance
     */
    virtual RealType type() = 0;

    /**
     * @return the name of that type, e.g. "int"
     */
    QString name() const;

    /**
     * @return id ID of this type, as given by objdump output
     */
    int id() const;

    /**
     * @return the size of this type in bytes
     */
    uint size() const;

    /**
     * @return a string representation of this type
     */
    virtual QString toString(size_t offset) const = 0;

//    template <typename T>
//    Instance value(size_t offset) const;

protected:
    const QString _name;    ///< name of this type, e.g. "int"
    const quint32 _id;      ///< ID of this type, given by objdump
    const quint32 _size;    ///< size of this type in byte
    BaseType* _parent;      ///< enclosing struct, if this is a struct member
};

#endif // BASETYPE_H
