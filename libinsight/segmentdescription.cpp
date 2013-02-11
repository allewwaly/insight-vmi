
#include <insight/segmentdescription.h>
#include <QSet>

static QSet<QString> segments;


SegmentDescription::SegmentDescription(const TypeInfo& info)
{
    setSegment(info.segment());
}

void SegmentDescription::setSegment(const QString &segment)
{
    if (segment.isEmpty())
        _segment.clear();
    else
        // Use a string with shared reference
        _segment = *segments.insert(segment);
}


void SegmentDescription::readFrom(KernelSymbolStream &in)
{
    // We added the segment information in version 19
    if (in.kSymVersion() >= 19) {
        in >> _segment;
        // Use a string with shared reference
        setSegment(_segment);
    }
    else
        _segment.clear();
}


void SegmentDescription::writeTo(KernelSymbolStream &out) const
{
    // We added the segment information in version 19
    if (out.kSymVersion() >= 19)
        out << _segment;
}

