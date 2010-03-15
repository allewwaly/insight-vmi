#ifndef INTEGER_H
#define INTEGER_H

class Integer : public BaseType
{
public:
    /**
     * Constructor
     * @param name name of this type, e.g. "int"
     * @param id ID of this type, as given by objdump output
     * @param size size of this type in bytes
     */
    Integer(const QString& name, const quint32 id, const quint32 size);

    /**
     * @return a string representation of this type
     */
//    virtual QString toString(size_t offset) const;

    /**
     * @return the native value of this type
     */
    int value(size_t offset) const;
};

#endif // INTEGER_H
