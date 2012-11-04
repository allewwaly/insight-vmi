#include "typerule.h"
#include "typefilter.h"
#include "osfilter.h"
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


QString TypeRule::toString() const
{
    QString s;
    static const QString indent(QString("\n%1").arg(str::filterIndent));

    if (!_name.isEmpty())
        s += "Name: " + _name + "\n";
    if (!_description.isEmpty())
        s += "Description: " + _description + "\n";
    if (_osFilter) {
        QString f(_osFilter->toString().trimmed());
        f.replace("\n", indent);
        s += "OS filter:" + indent + f + "\n";
    }
    if (_filter) {
        QString f(_filter->toString().trimmed());
        f.replace("\n", indent);
        s += "Type filter:" + indent + f + "\n";
    }
    if (!_action.isEmpty()) {
        s += "Action";
        if (_actionType == atFunction) {
            // Take absolute or relative file name, which ever is shorter
            QString file = QDir::current().relativeFilePath(_scriptFile);
            if (file.size() > _scriptFile.size())
                file = _scriptFile;

            s += ": call " + _action + "() in file \"";
            s += file + "\"\n";
        }
        else {
            QString a(_action);
            a.replace("\n", indent);
            s += " (inline):" + indent + a;
        }
    }
    return s;
}
