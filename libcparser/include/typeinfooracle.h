#ifndef TYPEINFOORACLE_H
#define TYPEINFOORACLE_H

#include <QString>

class ASTType;

/**
 * This abstract class can be used by ASTBuilder as an external source for
 * type information.
 */
class TypeInfoOracle
{
public:
	/**
	 * Checks if the given name represents a type.
	 * @param name type name to check
	 * @param realTypes bitwise OR'ed combination of accepted RealType values
	 * expected
	 * @return \c true if \a name is a type name, \c false otherwise
	 */
	virtual bool isTypeName(const QString& name, int types) const = 0;

	/**
	 * Retrievs the ASTType for the type identified by \a name. The caller is
	 * responsible for deleting the chain of returned objects afterwards!
	 * @param name type name to find the ASTtype for
	 * @param realTypes bitwise OR'ed combination of accepted RealType values
	 * expected
	 * @return the type, if \a name was found, \c null otherwise
	 */
	virtual ASTType* typeOfIdentifier(const QString& name, int types) const = 0;

	/**
	 * Retrieves the ASTType for member \a memberName within struct or union
	 * \a embeddingType.
	 * @param embeddingType the struct/union in which scope to search
	 * @param memberName the name of the member
	 * @return the ASTType of the member, if found, \c null otherwise
	 */
	virtual ASTType* typeOfMember(const ASTType* embeddingType,
								  const QString& memberName) const = 0;
};

#endif // TYPEINFOORACLE_H
