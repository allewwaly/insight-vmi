
#include <insight/memorysection.h>
#include <QSet>

static QSet<QString> sections;


MemorySection::MemorySection(const TypeInfo& info)
{
    setSection(info.section());
}

void MemorySection::setSection(const QString &section)
{
    if (section.isEmpty())
        _section.clear();
    else
        // Use a string with shared reference
        _section = *sections.insert(section);
}


void MemorySection::readFrom(KernelSymbolStream &in)
{
    // We added the segment information in version 19
    if (in.kSymVersion() >= 19) {
        in >> _section;
        // Use a string with shared reference
        setSection(_section);
    }
    else
        _section.clear();
}


void MemorySection::writeTo(KernelSymbolStream &out) const
{
    // We added the segment information in version 19
    if (out.kSymVersion() >= 19)
        out << _section;
}

