#include <insight/typerule.h>
#include <insight/typefilter.h>
#include <insight/typeruleparser.h>
#include <insight/osfilter.h>
#include <insight/shellutil.h>
#include <insight/basetype.h>
#include <insight/scriptengine.h>
#include <insight/ioexception.h>
#include <insight/filenotfoundexception.h>
#include <insight/typeruleengine.h>
#include <insight/typeruleexception.h>
#include <insight/astexpression.h>
#include <insight/astexpressionevaluator.h>
#include <insight/symfactory.h>
#include <insight/memspecs.h>
#include <insight/console.h>
#include <insight/instanceclass.h>
#include <insight/kernelsourcetypeevaluator.h>
#include <abstractsyntaxtree.h>
#include <astbuilder.h>
#include <astscopemanager.h>
#include <astnodefinder.h>
#include <QDir>
#include <QScriptProgram>

//------------------------------------------------------------------------------
// TypeRule
//------------------------------------------------------------------------------

TypeRule::TypeRule()
    : _filter(0), _osFilter(0), _action(0), _srcFileIndex(-1), _srcLine(0),
      _priority(0)

{
}


TypeRule::~TypeRule()
{
    if (_filter)
        delete _filter;
    if (_action)
        delete _action;
    // We don't create _osFilter, so don't delete it
}


void TypeRule::setFilter(const InstanceFilter *filter)
{
    if (_filter)
        delete _filter;
    _filter = filter;
}


void TypeRule::setAction(TypeRuleAction *action)
{
    if (_action)
        delete _action;
    _action = action;
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
                .arg(Console::colorize("Name:", ctColHead, col))
                .arg(_name);
    }
    if (!_description.isEmpty()) {
        s += QString("%1 %2\n")
                .arg(Console::colorize("Description:", ctColHead, col))
                .arg(_description);
    }
    s += QString("%1 %2\n")
            .arg(Console::colorize("Priority:", ctColHead, col))
            .arg(_priority);
    if (_osFilter) {
        QString f(_osFilter->toString(col).trimmed());
        f.replace("\n", indent);
        s += Console::colorize("OS filter:", ctColHead, col);
        s +=  indent + f + "\n";
    }
    if (_filter) {
        QString f(_filter->toString(col).trimmed());
        f.replace("\n", indent);
        s += Console::colorize("Type filter:", ctColHead, col);
        s +=  indent + f + "\n";
    }
    if (_action) {
        QString a(_action->toString(col).trimmed());
        a.replace("\n", indent);
        s += Console::colorize("Action:", ctColHead, col);
        s += indent + a + "\n";
    }
    return s;
}


//------------------------------------------------------------------------------
// ScriptAction
//------------------------------------------------------------------------------

ScriptAction::~ScriptAction()
{
    if (_program)
        delete _program;
}


Instance ScriptAction::evaluate(const Instance *inst,
                                const ConstMemberList &members,
                                ScriptEngine *eng, bool *matched) const
{
    // Parallel evaluation of threads
//    static QMutex lock(QMutex::Recursive);
//    QMutexLocker l(&lock);

    // Using the same QScriptProgram by multiple engines concurrently causes
    // a segmentation fault.
    // See https://bugreports.qt-project.org/browse/QTBUG-29246
    QScriptProgram prog(_program->sourceCode(), _program->fileName(),
                        _program->firstLineNumber());

    if (matched)
        *matched = true;

    if (!_program || !inst)
        return Instance();

    eng->initScriptEngine();

    // Instance passed to the rule as 1. argument
    QScriptValue instVal = InstanceClass::instToScriptValue(eng->engine(), *inst);
    // List of accessed member indices passed to the rule as 2. argument
    QScriptValue indexlist = eng->engine()->newArray(members.size());
    for (int i = 0; i < members.size(); ++i)
        indexlist.setProperty(i, eng->engine()->toScriptValue(members[i]->index()));

    QScriptValueList args;
    args << instVal << indexlist;
    QScriptValue ret(eng->evaluateFunction(funcToCall(), args, prog,
                                           _includePaths, inst->memDumpIndex()));

    if (eng->lastEvaluationFailed())
        warnEvalError(eng, prog.fileName());
    else if (ret.isBool() || ret.isNumber() || ret.isNull()) {
        if (matched)
            *matched = ret.toBool();
    }
    else if (ret.isArray()) {
        InstanceList list;
        qScriptValueToSequence(ret, list);
        return Instance::fromList(list);
    }
    else {
        Instance instRet;
        InstanceClass::instFromScriptValue(ret, instRet);
        return instRet;
    }

    return Instance();
}


void ScriptAction::warnEvalError(const ScriptEngine *eng,
                                 const QString &fileName) const
{
    static QMutex mutex;
    // Print errors as warnings
    if (eng && eng->lastEvaluationFailed()) {
        // Do this exclusively
        QMutexLocker lock(&mutex);

        QString file(ShellUtil::shortFileName(fileName));
        Console::err() << Console::color(ctWarning) << "At "
                     << Console::color(ctBold) << file
                     << Console::color(ctWarning);
        if (eng->hasUncaughtException()) {
            Console::err() << ":"
                         << Console::color(ctBold)
                         << eng->uncaughtExceptionLineNumber()
                         << Console::color(ctWarning);
        }
        Console::err() << ": " << eng->lastError()
                     << Console::color(ctReset) << endl;
    }
}


//------------------------------------------------------------------------------
// FuncCallScriptAction
//------------------------------------------------------------------------------

bool FuncCallScriptAction::check(const QString &xmlFile, const TypeRule *rule,
                                 SymFactory *factory)
{
    Q_UNUSED(rule);
    Q_UNUSED(factory);

    // Delete old program
    if (_program) {
        delete _program;
        _program = 0;
    }

    // Read the contents of the script file
    QFile scriptFile(_scriptFile);
    if (!scriptFile.open(QFile::ReadOnly))
        ioError(QString("Error opening file \"%1\" for reading.")
                    .arg(_scriptFile));
    _program = new QScriptProgram(scriptFile.readAll(), _scriptFile);

    // Basic syntax check
    QScriptSyntaxCheckResult result =
            QScriptEngine::checkSyntax(_program->sourceCode());
    if (result.state() != QScriptSyntaxCheckResult::Valid) {
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Syntax error in file %1 line %2 column %3: %4")
                               .arg(ShellUtil::shortFileName(_scriptFile))
                               .arg(result.errorLineNumber())
                               .arg(result.errorColumnNumber())
                               .arg(result.errorMessage()));
    }
    // Check if the function exists
    ScriptEngine eng(0);
    ScriptEngine::FuncExistsResult ret =
            eng.functionExists(_function, *_program);
    if (ret == ScriptEngine::feRuntimeError) {
        QString err;
        if (eng.hasUncaughtException()) {
            err = QString("Uncaught exception at line %0: %1")
                    .arg(eng.uncaughtExceptionLineNumber())
                    .arg(eng.lastError());
            QStringList bt = eng.uncaughtExceptionBacktrace();
            for (int i = 0; i < bt.size(); ++i)
                err += "\n    " + bt[i];
        }
        else
            err = eng.lastError();
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Runtime error in file %1: %2")
                               .arg(ShellUtil::shortFileName(_scriptFile))
                               .arg(err));
    }
    else if (ret == ScriptEngine::feDoesNotExist) {
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Function \"%1\" is not defined in file \"%2\".")
                               .arg(_function)
                               .arg(ShellUtil::shortFileName(_scriptFile)));
    }

    return true;
}


QString FuncCallScriptAction::toString(const ColorPalette *col) const
{
    QString file = ShellUtil::shortFileName(_scriptFile);

    return QString("Call %1() in file %2")
            .arg(Console::colorize(_function, ctBold, col))
            .arg(Console::colorize(file, ctBold, col));
}


//------------------------------------------------------------------------------
// ProgramScriptAction
//------------------------------------------------------------------------------

// static member variable
const QString ProgramScriptAction::_inlineFunc(js::inlinefunc);

bool ProgramScriptAction::check(const QString &xmlFile, const TypeRule *rule,
                                SymFactory *factory)
{
    Q_UNUSED(rule);
    Q_UNUSED(factory);

    // Delete old program
    if (_program) {
        delete _program;
        _program = 0;
    }

    QScriptSyntaxCheckResult result =
            QScriptEngine::checkSyntax(_srcCode);
    if (result.state() != QScriptSyntaxCheckResult::Valid) {
        typeRuleError2(xmlFile,
                       srcLine() + result.errorLineNumber() - 1,
                       result.errorColumnNumber(),
                       QString("Syntax error: %1")
                            .arg(result.errorMessage()));
    }
    else {
        // Wrap the code into a function so that the return statement
        // is available to the code
        QString code = QString("function %1() {\n%2\n}")
                .arg(js::inlinefunc).arg(_srcCode);
        _program = new QScriptProgram(code, xmlFile, srcLine() - 1);
    }

    return true;
}


QString ProgramScriptAction::toString(const ColorPalette *col) const
{
    Q_UNUSED(col);

    QStringList prog(_srcCode.split(QChar('\n')));
    // Remove empty lines at beginning and end
    while (!prog.isEmpty() && prog.first().trimmed().isEmpty())
        prog.pop_front();
    while (!prog.isEmpty() && prog.last().trimmed().isEmpty())
        prog.pop_back();
    if (prog.isEmpty())
        return QString();

    // Find indent in first line and remove it for all lines
    QString first(prog.first());
    int cnt = 0;
    const int tabWidth = 4;
    for (int i = 0; i < first.size(); ++i) {
        if (first[i] == QChar(' '))
            ++cnt;
        else if (first[i] == QChar('\t'))
            cnt += tabWidth;
        else break;
    }
    for (int i = 0; i < prog.size(); ++i) {
        int c = 0, pos = 0;
        while (c < cnt && pos < prog[i].size()) {
            if (prog[i][pos] == QChar(' '))
                ++c;
            else if (prog[i][pos] == QChar('\t'))
                c += tabWidth;
            else break;
            ++pos;
        }
        if (pos > 0)
            prog[i] = prog[i].mid(pos);
    }

    return prog.join("\n");
}


//------------------------------------------------------------------------------
// ExpressionAction
//------------------------------------------------------------------------------

ExpressionAction::~ExpressionAction()
{
    for (int i = 0; i < _exprList.size(); ++i)
        delete _exprList[i];
    _exprList.clear();
}


const BaseType* ExpressionAction::parseTypeStr(
        const QString &xmlFile, const TypeRule *rule, SymFactory *factory,
        const QString& what, const QString& typeStr, QString& id,
        bool *usesTypeId) const
{
    // Did we get a type name or a type ID?
    if (typeStr.startsWith("0x")) {
        if (usesTypeId)
            *usesTypeId = true;
        QStringList typeStrParts = typeStr.split(QRegExp("\\s+"));
        if (typeStrParts.isEmpty() || typeStrParts.size() > 2)
            typeRuleError2(xmlFile, srcLine(), -1,
                           QString("The specified %0 '%1' is invalid (expected "
                                   "a type ID and an identifier, e.g. '0x89ab "
                                   "src').")
                                .arg(what)
                                .arg(typeStr));

        QString typeIdStr = typeStrParts.first();
        bool ok = false;
        int typeId = typeIdStr.right(typeIdStr.size() - 2).toUInt(&ok, 16);
        if (!ok)
            typeRuleError2(xmlFile, srcLine(), -1,
                           QString("The type ID '%0' for the %1 is invalid.")
                                .arg(typeIdStr)
                                .arg(what));

        const BaseType* ret = factory->findBaseTypeById(typeId);
        if (!ret)
            typeRuleError2(xmlFile, srcLine(), -1,
                           QString("The type ID '%0' for the %1 does not exist.")
                                .arg(typeIdStr)
                                .arg(what));
        if (typeStrParts.size() > 1)
            id = typeStrParts.last();
        return ret;
    }
    else if (usesTypeId)
        *usesTypeId = false;

    // We got a type name expression, so parse it with the C parser
    AbstractSyntaxTree ast;
    ASTBuilder builder(&ast, factory);

    // Parse the code
    QByteArray code = QString("void %1(%2);").arg("__FUNCTION__").arg(typeStr).toAscii();
    if (builder.buildFrom(code) != 0)
        typeRuleError3(xmlFile, srcLine(), -1, TypeRuleException::ecSyntaxError,
                       QString("Syntax error in %0: %1")
                            .arg(what)
                            .arg(typeStr));

    QList<const ASTNode*> paramNodes =
            ASTNodeFinder::find(nt_parameter_declaration, &ast);
    // We might have more than one parameter for function pointer types, but the
    // first one is the one in __FUNCTION__()
    assert(paramNodes.size() > 0);
    const ASTNode* paramNode = paramNodes.first();

    // Do we have a declarator with or without an identifier?
    id.clear();
    const ASTNode* d_ad = paramNode->u.parameter_declaration.declarator_list ?
                paramNode->u.parameter_declaration.declarator_list->item : 0;
    if (d_ad && d_ad->type == nt_declarator)  {
        // Find the direct declaratior with identifier
        const ASTNode* dd = d_ad->u.declarator.direct_declarator;
        while (dd && !dd->u.direct_declarator.identifier) {
            dd = dd->u.direct_declarator.declarator
                   ->u.declarator.direct_declarator;
        }
        assert(dd != 0);
        assert(dd->u.direct_declarator.identifier);
        id = ast.antlrTokenToStr(dd->u.direct_declarator.identifier);
    }

    KernelSourceTypeEvaluator t_eval(&ast, factory);

    // Evaluate type of parameter_declaration
    ASTType* astType = t_eval.typeofNode(paramNode);
    if (!astType)
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Error parsing expression: %1").arg(QString(code)));

    // Search correspondig BaseType
    FoundBaseTypes found =
            factory->findBaseTypesForAstType(astType, &t_eval, false);
    if (found.types.isEmpty())
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Cannot find %1: %2").arg(what).arg(QString(code)));
    else if (found.types.size() > 1) {
        static const int type_ambiguous = -1;
        static const int no_type_found = -2;
        int match_idx = type_ambiguous;

        if (rule) {
            match_idx = no_type_found;
            // Find a unique type that matches the filter
            for (int i = 0; i < found.types.size(); ++i) {
                if (rule->match(found.types.at(i)) ||
                    rule->match(found.typesNonPtr.at(i)))
                {
                    if (match_idx < 0)
                        match_idx = i;
                    else {
                        const BaseType* t1 =
                                found.typesNonPtr.at(match_idx)->dereferencedBaseType(BaseType::trLexical);
                        const BaseType* t2 =
                                found.typesNonPtr.at(i)->dereferencedBaseType(BaseType::trLexical);
                        // Compare the hashes of the non-lexical types, only if
                        // they are different the type is ambiguous.
                        if (t1->hash() != t2->hash()) {
                            match_idx = type_ambiguous;
                            break;
                        }
                        else {
                            // Prefer the type that exactly corresponds to the
                            // ASTType
                            if (found.typesNonPtr.at(i)->type() == found.astTypeNonPtr->type())
                                match_idx = i;
                        }
                    }
                }
            }
        }

        if (match_idx >= 0)
            return found.types.at(match_idx);
        else if (match_idx == type_ambiguous)
            typeRuleError3(xmlFile, srcLine(), -1, TypeRuleException::ecTypeAmbiguous,
                           QString("The %1 is ambiguous, %2 types found for: %3")
                           .arg(what)
                           .arg(found.types.size())
                           .arg(typeStr));
        else
            typeRuleError3(xmlFile, srcLine(), -1, TypeRuleException::ecNotCompatible,
                           QString("Cannot find any compatible type for the %1: %2")
                           .arg(what).arg(typeStr));
    }
    else
        return found.types.first();

    return 0;
}


bool ExpressionAction::checkExprComplexity(const QString& xmlFile,
                                           const QString &what,
                                           const QString &expr) const
{
    static const QString illegal("{};\"?");
    QRegExp rx("[" + QRegExp::escape(illegal) + "]") ;
    if (expr.contains(rx))
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("The %1 contains one of the following "
                               "unsupported characters: %2")
                            .arg(what)
                            .arg(illegal));
    return true;
}


bool ExpressionAction::check(const QString &xmlFile, const TypeRule *rule,
                             SymFactory *factory)
{
    for (int i = 0; i < _exprList.size(); ++i)
        delete _exprList[i];
    _exprList.clear();
    _srcType = 0;
    _targetType = 0;
    _expr = 0;

    checkExprComplexity(xmlFile, _srcTypeStr, "source type");
    checkExprComplexity(xmlFile, _targetTypeStr, "target type");
    checkExprComplexity(xmlFile, _exprStr, "expression");

    QString srcId;

    // Check target type
    bool targetUsesId = false;
    _targetType = parseTypeStr(xmlFile, 0, factory, "target type",
                               _targetTypeStr, srcId, &targetUsesId);

    if (_targetType)
        _targetType = _targetType->dereferencedBaseType(BaseType::trLexical);

    // Check source type
    bool srcUsesId = false;
    _srcType = parseTypeStr(xmlFile, rule, factory, "source type",
                            _srcTypeStr, srcId, &srcUsesId);

    // Is the srcId valid?
    if (srcId.isEmpty())
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("The source type does not specify an identifier:"
                               " %1")
                            .arg(_srcTypeStr));

    // Make sure the expression contains the srcId
    if (!_exprStr.contains(QRegExp("\\b" + srcId + "\\b")))
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("The expression does not use identifier \"%1\" "
                               "which was defined in the source type.")
                            .arg(srcId));

    // Check complete expression
    AbstractSyntaxTree ast;
    ASTBuilder builder(&ast, factory);
    QString codeStr;
    // If the type was specified via ID, we have to use the pseudo typedef here
    if (srcUsesId)
        codeStr = QString("__0x%0__ %1;").arg((uint)_srcType->id(), 0, 16).arg(srcId);
    else
        codeStr = _srcTypeStr + ";";
    codeStr += " int __dest__ = " + _exprStr + ";";
    if (builder.buildFrom(codeStr.toAscii()) != 0)
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Syntax error in expression: %1")
                            .arg(codeStr));

    // We should finde exatcely one initializer
    QList<const ASTNode*> init_nodes =
            ASTNodeFinder::find(nt_initializer, &ast);
    if (init_nodes.isEmpty())
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Error parsing expression: %1")
                            .arg(codeStr));

    // Try to evaluate expression
    ASTTypeEvaluator t_eval(&ast, factory->memSpecs().sizeofLong,
                            factory->memSpecs().sizeofPointer, factory);
    ASTExpressionEvaluator e_eval(&t_eval, factory);
    ASTNodeNodeHash ptsTo;
    _expr = e_eval.exprOfNode(init_nodes.first(), ptsTo);
    if (_expr)
        // Expression belongs to the evaluator, we need to clone it
        _expr = _expr->copy(_exprList);
    else
        typeRuleError2(xmlFile, srcLine(), -1,
                       QString("Error evaluating expression: %1")
                            .arg(QString(_exprStr)));

    _altRefType.setId(_targetType->id());
    _altRefType.setExpr(_expr);

    return true;
}


bool ExpressionAction::match(const BaseType *type, const OsSpecs *specs) const
{
    Q_UNUSED(specs);
    if (!type || !_srcType)
        return false;

    // The given type must match the target type, but be forgiving with
    // pointer source types since the instance typically is dereferenced.
    const BaseType *srcType = _srcType, *last = _srcType, *typeNonArray;
    type = type->dereferencedBaseType(BaseType::trLexical);
    typeNonArray = type->dereferencedBaseType(BaseType::trLexicalAndPointers);
    typeNonArray = typeNonArray->type() == rtArray ?
                typeNonArray->dereferencedBaseType(rtArray, 1) : 0;

    int ptrsDerefed = 0;

    while (srcType &&
           ( (*type != *srcType) &&
             !(type->type() == rtArray && srcType->type() == rtArray &&
               // For array types, we ignore the array length by comparing the
               // arrays' referencing type directly
               typeNonArray &&
               (*typeNonArray == *srcType->dereferencedBaseType(rtArray, 1)) ) ) )
    {
        // Can we dereference the source type?
        if (srcType->type() & BaseType::trLexicalAndPointers) {
            last = srcType;
            int cnt = 0;
            srcType = srcType->dereferencedBaseType(BaseType::trLexicalAndPointers, 1, &cnt);
            ptrsDerefed += cnt;
            // void* pointers are not dereferenced anymore
            if (srcType == last)
                srcType = 0;
        }
        // If we dereferenced the source type, we can try to dereference the
        // tested type as well
        else if (ptrsDerefed > 0 && type->type() == rtPointer) {
            int cnt = 0;
            type = type->dereferencedBaseType(BaseType::trLexicalAndPointers,
                                              ptrsDerefed, &cnt);
            if (ptrsDerefed != cnt)
                return false;
            ptrsDerefed = 0;
        }
        else
            return false;
    }
    return srcType != 0;
}


Instance ExpressionAction::evaluate(const Instance *inst,
                                    const ConstMemberList &members,
                                    ScriptEngine *eng, bool *matched) const
{
    Q_UNUSED(eng);

    if (matched)
        *matched = true;

    if (!inst || !inst->type() || !_targetType || !_srcType || !_expr)
        return Instance();

    // The instance's type must match our source type
    if (!match(inst->type()))
        return Instance();

    const SymFactory* fac = inst->type()->factory();
    QString name = members.isEmpty() ? QString() : members.last()->name();
    QStringList parentNames(inst->fullNameComponents());
    // If we have more than one member access, add all other names to the parent
    // list
    for (int i = 0; i + 1 < members.size(); ++i)
        if (!members[i]->name().isEmpty())
            parentNames += members[i]->name();

    return _altRefType.toInstance(inst->vmem(), inst, fac, name, parentNames);
}


QString ExpressionAction::toString(const ColorPalette *col) const
{
    QString s;

    s += Console::colorize("Source type:", ctColHead, col) + " ";
    if (_srcType) {
        QString id = QString("0x%1").arg((uint)_srcType->id(), -8, 16);
        if (col)
            s += col->color(ctTypeId) + id + col->color(ctReset) + " " +
                    col->prettyNameInColor(_srcType, 0);
        else
            s += id + " " + _srcType->prettyName();
    }
    else
        s += _srcTypeStr;

    s += "\n" + Console::colorize("Target type:", ctColHead, col) + " ";
    if (_targetType) {
        QString id = QString("0x%1").arg((uint)_targetType->id(), -8, 16);
        if (col)
            s += col->color(ctTypeId) + id + col->color(ctReset) + " " +
                    col->prettyNameInColor(_targetType, 0);
        else
            s += id + " " + _targetType->prettyName();
    }
    else
        s += _targetTypeStr;

    s += "\n" + Console::colorize("Expression:", ctColHead, col) + "  ";
    s += _expr ? _expr->toString() : _exprStr;
    return s;
}


bool TypeRuleAction::match(const BaseType *type, const OsSpecs *specs) const
{
    Q_UNUSED(type);
    Q_UNUSED(specs);
    return true;
}


const QStringList &TypeRuleAction::supportedActionTypes()
{
    static QStringList types;
    if (types.isEmpty())
        types << xml::expression  << xml::inlineCode << xml::function;
    return types;
}


TypeRuleAction::ActionType TypeRuleAction::strToActionType(const QString& action)
{
    if (action == xml::expression)
        return TypeRuleAction::atExpression;
    else if (action == xml::function)
        return TypeRuleAction::atFunction;
    else if (action == xml::inlineCode)
        return TypeRuleAction::atInlineCode;
    else
        return TypeRuleAction::atNone;
}



