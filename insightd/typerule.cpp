#include "typerule.h"
#include "typefilter.h"
#include "osfilter.h"
#include "shellutil.h"
#include <QDir>


TypeRule::TypeRule()
    : _filter(0), _osFilter(0), _actionType(atNone), _srcFileIndex(-1),
      _srcLine(0), _actionSrcLine(0)
{
}


TypeRule::~TypeRule()
{
    if (_filter)
        delete _filter;
    // We don't create _osFilter, so don't delete it
}


void TypeRule::setFilter(const InstanceFilter *filter)
{
    if (_filter)
        delete _filter;
    _filter = filter;
}


bool TypeRule::match(const BaseType *type, const OsSpecs *specs) const
{
    if (_osFilter && !_osFilter->match(specs))
        return false;
    if (_filter && !_filter->matchType(type))
        return false;
    return true;
}


bool TypeRule::match(const Variable *var, const OsSpecs *specs) const
{
    if (_osFilter && !_osFilter->match(specs))
        return false;
    if (_filter && !_filter->matchVar(var))
        return false;
    return true;
}


bool TypeRule::match(const Instance *inst, const OsSpecs *specs) const
{
    if (_osFilter && !_osFilter->match(specs))
        return false;
    if (_filter && !_filter->matchInst(inst))
        return false;
    return true;
}


QString TypeRule::toString(const ColorPalette *col) const
{
    QString s;
    static const QString indent(QString("\n%1").arg(str::filterIndent));

    if (!_name.isEmpty()) {
        s += QString("%1 %2\n")
                .arg(ShellUtil::colorize("Name:", ctColHead, col))
                .arg(_name);
    }
    if (!_description.isEmpty()) {
        s += QString("%1 %2\n")
                .arg(ShellUtil::colorize("Description:", ctColHead, col))
                .arg(_description);
    }
    if (_osFilter) {
        QString f(_osFilter->toString(col).trimmed());
        f.replace("\n", indent);
        s += ShellUtil::colorize("OS filter:", ctColHead, col);
        s +=  indent + f + "\n";
    }
    if (_filter) {
        QString f(_filter->toString(col).trimmed());
        f.replace("\n", indent);
        s += ShellUtil::colorize("Type filter:", ctColHead, col);
        s +=  indent + f + "\n";
    }
    if (!_action.isEmpty()) {
        if (_actionType == atFunction) {
            s += ShellUtil::colorize("Action:", ctColHead, col);
            // Take absolute or relative file name, which ever is shorter
            QString file = QDir::current().relativeFilePath(_scriptFile);
            if (file.size() > _scriptFile.size())
                file = _scriptFile;

            s += " call " + ShellUtil::colorize(_action + "()", ctBold, col) +
                    " in file " + ShellUtil::colorize(file, ctBold, col) + "\n";
        }
        else {
            QString a(_action);
            a.replace("\n", indent);
            s += ShellUtil::colorize("Action (inline):", ctColHead, col);
            s += indent + a;
        }
    }
    return s;
}
