#ifndef SEGMENTDESCRIPTION_H
#define SEGMENTDESCRIPTION_H

#include <QString>
#include "typeinfo.h"
#include "kernelsymbolstream.h"

/**
 * This interface class adds a segment name to the derived class.
 */
class SegmentDescription
{
public:
	/**
	 * Constructor
	 */
	SegmentDescription() {}

	/**
	 * Constructor
	 * @param info type information to initialize from
	 */
	SegmentDescription(const TypeInfo& info);

	/**
	 * Returns the segment where this variable is stored.
	 * @return name of the segment
	 * \sa setSegment()
	 */
	inline const QString& segment() const
	{
		return _segment;
	}

	/**
	 * Sets the segment where this variable is stored.
	 * @param segment segment name
	 * \sa segement()
	 */
	void setSegment(const QString& segment);

	/**
	 * Reads a serialized version of this object from \a in.
	 * \sa writeTo()
	 * @param in the data stream to read the data from, must be ready to read
	 */
	void readFrom(KernelSymbolStream &in);

	/**
	 * Writes a serialized version of this object to \a out
	 * \sa readFrom()
	 * @param out the data stream to write the data to, must be ready to write
	 */
	void writeTo(KernelSymbolStream& out) const;

private:
	QString _segment;
};

#endif // SEGMENTDESCRIPTION_H
