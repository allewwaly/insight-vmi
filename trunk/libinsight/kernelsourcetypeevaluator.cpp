/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include <insight/kernelsourcetypeevaluator.h>
#include <insight/symfactory.h>
#include <debug.h>
#include <bugreport.h>
#include <astnode.h>
#include <astscopemanager.h>
#include <astsourceprinter.h>
#include <abstractsyntaxtree.h>
#include <insight/console.h>
#include <insight/astexpressionevaluator.h>
#include "expressionevalexception.h"
#include "typeevaluatorexception.h"

#define sourceTypeEvaluatorError(x) do { throw SourceTypeEvaluatorException((x), __FILE__, __LINE__); } while (0)

KernelSourceTypeEvaluator::KernelSourceTypeEvaluator(AbstractSyntaxTree* ast,
        SymFactory* factory)
    : ASTTypeEvaluator(ast,
                       factory->memSpecs().sizeofLong,
                       factory->memSpecs().sizeofPointer),
      _factory(factory), _eval(0)
{
    _eval = new ASTExpressionEvaluator(this, _factory);
}


KernelSourceTypeEvaluator::~KernelSourceTypeEvaluator()
{
    if (_eval)
        delete _eval;
}


void KernelSourceTypeEvaluator::primaryExpressionTypeChange(
        const TypeEvalDetails &ed)
{
    // Ignore all usages of a pointer as an integer, we cannot learn anything
    // from that
    if (!(ed.targetType->type() & (rtArray|rtPointer))) {
//        debugmsg("Target is no pointer:\n" +
//                 typeChangeInfo(ed));
        /// @todo Consider function pointers as target type
        return;
    }
    // Ignore changes from any pointer to a void pointer
    if (ed.targetType->type() == rtPointer &&
        ed.targetType->next()->type() == rtVoid)
    {
        return;
    }
    // Ignore function parameters of non-struct source types as source
    if (ed.sym->type() == stFunctionParam && !ed.transformations.memberCount()) {
//        debugmsg("Source is a paramter without struct member reference:\n" +
//                 typeChangeInfo(ed));
        return;
    }
    // Ignore local variables of non-struct source types as source
    if ((ed.sym->type() == stVariableDecl ||
         ed.sym->type() == stVariableDef)
            && ed.sym->isLocal() && !ed.transformations.memberCount())
    {
//        debugmsg("Source is a local variable without struct member reference:\n" +
//                 typeChangeInfo(ed));
        return;
    }
    // Ignore values return by functions
    if (ed.sym->type() == stFunctionDef ||
        ed.sym->type() == stFunctionDecl)
    {
//        debugmsg("Source is return value of function invocation:\n" +
//                 typeChangeInfo(ed));
        return;
    }

    /// @todo Ignore casts from arrays to pointers of the same base type

    try {
#ifdef DEBUG_APPLY_USED_AS
        // Find top-level node of right-hand side for expression
        const ASTNode* right = ed.srcNode;
        while (right && right->parent != ed.rootNode) {
            if (ed.interLinks.contains(right))
                right = ed.interLinks[right];
            else
                right = right->parent;
        }
        // Prepare the right-hand side expression
        ASTNodeNodeHash ptsTo = invertHash(ed.interLinks);
        ASTExpression* expr = 0;
        try {
            expr = right ? _eval->exprOfNode(right, ptsTo) : 0;
        }
        catch (ExpressionEvalException& e) {
            // do nothing
        }
        QString s_expr = expr ? expr->toString() : QString("n/a");

        debugmsg("Passing the following type change to SymFactory:\n" +
                 typeChangeInfo(ed, s_expr));
#endif

        _factory->typeAlternateUsage(&ed, this);
    }
    catch (FactoryException& e) {
        // Print the source of the embedding external declaration
        const ASTNode* n = ed.srcNode;
        while (n && n->parent && n->type != nt_function_definition)
            n = n->parent;
        reportErr(e, n, &ed);
    }
}


bool KernelSourceTypeEvaluator::interrupted() const
{
    return Console::interrupted();
}


int KernelSourceTypeEvaluator::evaluateIntExpression(const ASTNode* node, bool* ok)
{
    if (ok)
        *ok = false;

    if (node) {
        ASTExpression* expr = 0;
        try {
            expr = _eval->exprOfNode(node, ASTNodeNodeHash());
        }
        catch (ExpressionEvalException& e) {
            // Fail silently if a required type could not be found
            if (e.errorCode == ExpressionEvalException::ecTypeNotFound)
                return 0;
            else
                throw;
        }

        ExpressionResult value = expr->result();

        // Return constant value
        if (value.resultType == erConstant) {
            // Consider it to be an error if the expression evaluates to float
            if (ok)
                *ok = (value.size & esInteger);
            return value.value();
        }
        // A constant value may still be undefined for missing type information.
        // We should be able to evaluate all other constant expressions that
        // don't have runtime dependencies!
        else if (! (value.resultType & (erRuntime|erUndefined|erGlobalVar|erLocalVar)) )
        {
            ASTSourcePrinter printer(_ast);
            sourceTypeEvaluatorError(
                        QString("Failed to evaluate constant expression "
                                "\"%1\" at %2:%3:%4")
                        .arg(printer.toString(node)
                             .trimmed())
                        .arg(_ast ? _ast->fileName() : QString("-"))
                        .arg(node->start->line)
                        .arg(node->start->charPosition));
        }
    }

    return 0;
}

//#define DEBUGMAGICNUMBERS   1

void KernelSourceTypeEvaluator::evaluateMagicNumbers_constant(const ASTNode *node, 
            bool *intConst, qint64 *resultInt, 
            bool *stringConst, QString *resultString,
            QString &string)
{
    ASTSourcePrinter printer(_ast);

    *intConst = false;
    *stringConst = false;

    switch(node->type){
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Node Type: %1\n").arg(ast_node_type_to_str(node)));
#endif /* DEBUGMAGICNUMBERS */
    
        case nt_unary_expression_op:
        case nt_unary_expression_builtin: //TODO Maybe implement?
        case nt_builtin_function_sizeof:
        case nt_unary_expression_inc:
        case nt_unary_expression_dec:
            return;
        
        case nt_conditional_expression:
            if(node->u.conditional_expression.logical_or_expression)
                evaluateMagicNumbers_constant(node->u.conditional_expression.logical_or_expression, intConst, resultInt,
                    stringConst, resultString, string);
            if(node->u.conditional_expression.conditional_expression)
                evaluateMagicNumbers_constant(node->u.conditional_expression.conditional_expression, intConst, resultInt,
                    stringConst, resultString, string);
            return;

        case nt_logical_or_expression:
        case nt_logical_and_expression:
        case nt_inclusive_or_expression:
        case nt_exclusive_or_expression:
        case nt_and_expression:
        case nt_equality_expression:
        case nt_relational_expression:
        case nt_shift_expression:
        case nt_additive_expression:
        case nt_multiplicative_expression:
            evaluateMagicNumbers_constant(node->u.binary_expression.left, intConst, resultInt,
                    stringConst, resultString, string);
            if (!node->u.binary_expression.right) 
                return;
            else if (*stringConst){
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Stringconstant with operation?\n"));
                string.append(QString("Node was: %1\n").arg(printer.toString(node, false).trimmed()));
                debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                return;
            }else if (*intConst){
                //TODO Evaluate right side and calculate operation
                
                bool rightIntConst = false;
                qint64 rightResultInt = 0;
                evaluateMagicNumbers_constant(node->u.binary_expression.right, &rightIntConst, &rightResultInt,
                        stringConst, resultString, string);
                if (!rightIntConst){
                    *intConst = false;
                    return;
                }

                QString op;

                switch (node->type) {
                    case nt_logical_or_expression:
                        *resultInt = *resultInt || rightResultInt;
                        break;
                    case nt_logical_and_expression:
                        *resultInt = *resultInt && rightResultInt;
                        break;
                    case nt_inclusive_or_expression:
                        *resultInt = *resultInt | rightResultInt;
                        break;
                    case nt_exclusive_or_expression:
                        *resultInt = *resultInt ^ rightResultInt;
                        break;
                    case nt_and_expression:
                        *resultInt = *resultInt & rightResultInt;
                        break;
                    case nt_shift_expression:
                        op = antlrTokenToStr(node->u.binary_expression.op);
                        if (op == "<<")
                            *resultInt = *resultInt << rightResultInt;
                        else
                            *resultInt = *resultInt >> rightResultInt;
                        break;
                    case nt_additive_expression:
                        op = antlrTokenToStr(node->u.binary_expression.op);
                        if (op == "+")
                            *resultInt = *resultInt + rightResultInt;
                        else
                            *resultInt = *resultInt - rightResultInt;
                        break;
                    case nt_multiplicative_expression:
                        op = antlrTokenToStr(node->u.binary_expression.op);
                        if (op == "*")
                            *resultInt = *resultInt * rightResultInt;
                        else if (op == "/")
                            *resultInt = *resultInt / rightResultInt;
                        else
                            *resultInt = *resultInt % rightResultInt;
                        break;
                    default:
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString("Type represents no binary expression\n"));
#endif /* DEBUGMAGICNUMBERS */
                        break;
                }
                return;
            }
            return;
        case nt_cast_expression:
            if (node->u.cast_expression.unary_expression)
                evaluateMagicNumbers_constant(node->u.cast_expression.unary_expression, intConst, resultInt,
                    stringConst, resultString, string);
            else if (node->u.cast_expression.cast_expression)
                evaluateMagicNumbers_constant(node->u.cast_expression.cast_expression, intConst, resultInt,
                    stringConst, resultString, string);
            return;
        case nt_unary_expression:
            evaluateMagicNumbers_constant(node->u.unary_expression.postfix_expression, intConst, resultInt,
                    stringConst, resultString, string);
            return;
        case nt_postfix_expression:
            evaluateMagicNumbers_constant(node->u.postfix_expression.primary_expression, intConst, resultInt,
                    stringConst, resultString, string);
            return;
        case nt_primary_expression:
            
            if (node->u.primary_expression.constant)
            {
                QString constant = printer.toString(node->u.primary_expression.constant,false).trimmed();
                if (!constant.isNull())
                {
                    ASTType* constantType;
                    try{
                        constantType = typeofNode(node);
                    } catch (TypeEvaluatorException&) {
                        return;
                    }
                    if(constantType->type() == rtArray && 
                            constantType->next() &&
                            constantType->next()->type() == rtInt8)
                    {
                        //Interesting as this could be names of modules/structs
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString("Found string constant!! \"%1\"").arg(constant));
#endif /* DEBUGMAGICNUMBERS */
                        *stringConst = true;
                        *resultString = constant;
                        return;
                    }
                    else if (constantType->type() & IntegerTypes)
                    {
                        //Try to parse integer as it could be interesting
                        //Cut off L
                        if(constant.indexOf("U", 0, Qt::CaseInsensitive) > -1)
                            constant.truncate(constant.indexOf("U", 0, Qt::CaseInsensitive));
                        if(constant.indexOf("L", 0, Qt::CaseInsensitive) > -1)
                            constant.truncate(constant.indexOf("L", 0, Qt::CaseInsensitive));

                        *resultInt = constant.toInt( intConst, 0 );

#ifdef DEBUGMAGICNUMBERS
                        string.append(QString("Found int32 constant: %1\n").arg(*resultInt));
#endif /* DEBUGMAGICNUMBERS */
                    }
                    //TODO also parse (u)int[16|64] (LL/ULL)
#ifdef DEBUGMAGICNUMBERS
                    else
                    {
                        RealType type = constantType->type();
                        string.append(QString("Found constant!! %1 : %2 : %3.")
                                .arg(realTypeToStr(type))
                                .arg(constantType->identifier())
                                .arg((constantType->next()) ? realTypeToStr(constantType->next()->type()): "?")
                                );
                    }
#endif /* DEBUGMAGICNUMBERS */
                }
            }
            return;

        default:
#ifdef DEBUGMAGICNUMBERS
            string.append(QString("Unknown ASTType while going downwards: %1\n").arg(ast_node_type_to_str(node)));
            string.append(QString("Node was: %1\n").arg(printer.toString(node, false).trimmed()));
            debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
            return;
    }
}

void KernelSourceTypeEvaluator::evaluateMagicNumbers_initializer(
        const ASTNode *node, const Structured* structured, QString& string){
    bool intConst = false;
    qint64 resultInt = 0;
    bool stringConst = false;
    QString resultString;

    QString memberName;

    if (!node->type == nt_initializer && node->u.initializer.assignment_expression) return;

    ASTSourcePrinter printer(_ast);
    ASTNode *localNode = node->u.initializer.assignment_expression;

    if(localNode->u.assignment_expression.designated_initializer_list){

        if(!localNode->u.assignment_expression.designated_initializer_list->item->u.designated_initializer.identifier){
#ifdef DEBUGMAGICNUMBERS
            string.append(QString("\tConstant1: %1\n\tConstant2: %2\n\tIdentifier: %3\n")
                    .arg(printer.toString(localNode->u.assignment_expression.designated_initializer_list->item->u.designated_initializer.constant_expression1, false).trimmed())
                    .arg(printer.toString(localNode->u.assignment_expression.designated_initializer_list->item->u.designated_initializer.constant_expression2, false).trimmed())
                    .arg(antlrTokenToStr(localNode->u.assignment_expression.designated_initializer_list->item->u.designated_initializer.identifier).trimmed())
                    );
#endif /* DEBUGMAGICNUMBERS */
            return;
        }

        memberName = antlrTokenToStr(localNode->u.assignment_expression.designated_initializer_list->item->u.designated_initializer.identifier).trimmed();
    }
    
    if (!memberName.isEmpty()){
        StructuredMember *member = const_cast<StructuredMember*>(structured->member(memberName));
        if(member){
            //Member is not constant
            if(member->hasNotConstValue()){
                return;
            }
            //Member may have constant value
            else if(localNode->u.assignment_expression.initializer &&
                    localNode->u.assignment_expression.initializer->u.initializer.assignment_expression &&
                    localNode->u.assignment_expression.initializer->u.initializer.assignment_expression
                             ->u.assignment_expression.conditional_expression)
            {
                evaluateMagicNumbers_constant(localNode->u.assignment_expression.initializer
                        ->u.initializer.assignment_expression
                        ->u.assignment_expression.conditional_expression,
                        &intConst, &resultInt, 
                        &stringConst, &resultString,
                        string);
            }
            //Member itself is a struct with an own initializer
            else if(localNode->u.assignment_expression.initializer &&
                    localNode->u.assignment_expression.initializer->u.initializer.initializer_list){
                //Have a look on internal object

                // Dereference all lexical types (typedef/const/volatile)
                const Structured* memberStruct =
                        dynamic_cast<const Structured*>(member->refTypeDeep(BaseType::trLexical));

                // Make sure the pointer is valid!
                if (!memberStruct) {
#ifdef DEBUGMAGICNUMBERS
                    debugerr(QString("Member \"%1\" in %2 (0x%3) is not a struct/union, but \"%4\".")
                             .arg(memberName)
                             .arg(structured->prettyName())
                             .arg((uint)structured->id(), 0, 16)
                             .arg(member->refTypeDeep(BaseType::trLexical)->prettyName()));
#endif /* DEBUGMAGICNUMBERS */
                    return;
                }

                ASTNodeList* initList = localNode->u.assignment_expression.initializer->u.initializer.initializer_list;
                while(initList){
                    if(initList->item->u.initializer.assignment_expression){
                        evaluateMagicNumbers_initializer(initList->item, memberStruct, string);
                    }else{
                        //TODO debug!
                    }
                    initList = initList->next;
                }
                return;
            }

            if ((!intConst && !stringConst)){
                member->evaluateMagicNumberFoundNotConstant();
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Found not constant\n"));
#endif /* DEBUGMAGICNUMBERS */
            } else if(intConst){
                member->evaluateMagicNumberFoundInt(resultInt);
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Found constant Int: %1\n").arg(resultInt));
#endif /* DEBUGMAGICNUMBERS */
            } else if (stringConst) {
                member->evaluateMagicNumberFoundString(resultString);
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Found constant String: %1\n").arg(resultString));
#endif /* DEBUGMAGICNUMBERS */
            }
        }
    }

    return;
}

void KernelSourceTypeEvaluator::evaluateMagicNumbers(const ASTNode *node)
{
    if(!node) return;

    //Search for type // struct
    const Structured* structured = 0;
    StructuredMember* member = 0;
    const ASTNode* localNode;

    QString string;

#ifdef DEBUGMAGICNUMBERS
    ASTSourcePrinter printer(_ast);
#endif /* DEBUGMAGICNUMBERS */

    if (node->type == nt_init_declarator &&
        node->parent)
    {
#ifdef DEBUGMAGICNUMBERS
        string.append(QString("\nCurrent Node: %1 \n")
                .arg(printer.toString(node, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */
        ASTNode* declarator = node->u.init_declarator.declarator;
        ASTType* constantType;
        try{
            constantType = typeofNode(declarator);
        }catch(TypeEvaluatorException&){
            return;
        }

        while ((constantType->type() & RefBaseTypes) && 
                constantType->next())
        {
            constantType = constantType->next();
#ifdef DEBUGMAGICNUMBERS
            string.append(QString("ReferenceType with identifier: %1\n")
                    .arg(constantType->identifier()));
#endif /* DEBUGMAGICNUMBERS */
        }

        if (constantType->identifier().isEmpty()) return;

        BaseType* baseType = _factory->findBaseTypeByName(constantType->identifier());
        if (!baseType || !(baseType->type() & (rtStruct | rtUnion))){
            return;
        }
                
        structured = dynamic_cast<const Structured*>(baseType);
        
        ASTNode* initializer = node->u.init_declarator.initializer;
        if (!initializer || initializer->type != nt_initializer) return;

        if(initializer->u.initializer.initializer_list){
            ASTNodeList* initList = initializer->u.initializer.initializer_list;
            while(initList){
                if(initList->item->u.initializer.assignment_expression){
                    evaluateMagicNumbers_initializer(initList->item, structured, string);
                }else{
                    //TODO debug!
                }
                initList = initList->next;
            }

        }
        else if (initializer->u.initializer.assignment_expression){
            evaluateMagicNumbers_initializer(initializer, structured, string);
        }
    }

    else if (node->type == nt_primary_expression &&
        node->parent &&
        node->parent->type == nt_postfix_expression)
    {
#ifdef DEBUGMAGICNUMBERS
        string.append(QString("\nCurrent Node: %1 \n")
                .arg(printer.toString(node, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */

        ASTType* constantType;
        try{
            constantType = typeofNode(node);
        }catch(TypeEvaluatorException&){
            return;
        }

        while ((constantType->type() & RefBaseTypes) && 
                constantType->next())
        {
            constantType = constantType->next();
#ifdef DEBUGMAGICNUMBERS
            string.append(QString("ReferenceType with identifier: %1\n")
                    .arg(constantType->identifier()));
#endif /* DEBUGMAGICNUMBERS */
        }

        if (constantType->type() & (IntegerTypes | rtVoid))
        {
            //Not interesting
            return;
        }
        else if (constantType->type() == rtStruct || constantType->type() == rtUnion)
        {
#ifdef DEBUGMAGICNUMBERS
            string.append(QString("Found Struct to %1\n")
                    .arg(constantType->identifier().trimmed()));
#endif /* DEBUGMAGICNUMBERS */

            const ASTNodeList* list;

            if (!constantType->identifier().isEmpty())
            {
                //Find member in primaryNode to identify structured member
                //Handle Expression suffixes
                if (node->parent->u.postfix_expression.postfix_expression_suffix_list)
                {
                    QString memberName;

                    list = node->parent->u.postfix_expression.postfix_expression_suffix_list;
                    while(list){
                        switch(list->item->type){
                            case nt_postfix_expression_arrow:
                            case nt_postfix_expression_dot:
                                if(!antlrTokenToStr(list->item->u.postfix_expression_suffix.identifier).isEmpty()){
                                    memberName = antlrTokenToStr(list->item->u.postfix_expression_suffix.identifier).trimmed();
#ifdef DEBUGMAGICNUMBERS
                                    string.append(QString("Found Membername: %1\n")
                                            .arg(memberName));
#endif /* DEBUGMAGICNUMBERS */
                                    if(!structured){
                                        const BaseType* baseType = _factory->findBaseTypeByName(constantType->identifier());
                                        if (baseType && baseType->type() & (rtStruct|rtUnion))
                                        {
                                            structured = dynamic_cast<const Structured*>(baseType);
                                            member = (StructuredMember*) structured->member(memberName);
                                            if (member){
                                                if (member->refType()->type() == rtStruct || member->refType()->type() ==rtUnion)
                                                    structured = dynamic_cast<const Structured*>(member->refType());
                                                break;
                                            }
                                        }
                                    } else {
                                        member = (StructuredMember*) structured->member(memberName);
                                        if (member){
                                            if (member->refType()->type() & (rtStruct | rtUnion))
                                                structured = dynamic_cast<const Structured*>(member->refType());
                                        }
                                    }
                                }else return;
                                // Fall through intentional!
                            case nt_postfix_expression_parens:
                            case nt_postfix_expression_brackets:
                                list = list->next;
                                break;

                            case nt_postfix_expression_inc:
                            case nt_postfix_expression_dec:
                                if (member) member->evaluateMagicNumberFoundNotConstant();
                                return;

                            default:
#ifdef DEBUGMAGICNUMBERS
                                string.append(QString("Unknown Type: %1\n").arg(ast_node_type_to_str(list->item)));
                                debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                                return;
                        }
                    }
                    if (!structured)
                    {
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString("UNKNOWN STRUCT: %1 with member %2 - BUG?")
                                .arg(constantType->identifier())
                                .arg(memberName)
                                );
                        //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                        return;
                    }
                }
                else
                {
#ifdef DEBUGMAGICNUMBERS
                    string.append(QString("Struct without member?\n"));
                    //debugmsg(string); //TODO Maybe handle
#endif /* DEBUGMAGICNUMBERS */
                    return;
                }

                if (!member)
                {
#ifdef DEBUGMAGICNUMBERS
                    string.append("Invalid Member!!");
                    //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                    return;
                }
            }
            else
            {
                //Struct got no identifier, so we do not know the type
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Struct without name?"));
                //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                return;
            }
        }
#ifdef DEBUGMAGICNUMBERS
        else
        {
            string.append(QString("Found Type: %1\n").arg(realTypeToStr(constantType->type())));
        }
#endif /* DEBUGMAGICNUMBERS */

        if (!node || !member)
            return;

        localNode = node;
        bool findAssignment = true;
        while (findAssignment && localNode->parent)
        {
            switch(localNode->parent->type)
            {
                case nt_postfix_expression:
                case nt_unary_expression:
                case nt_cast_expression:
                case nt_lvalue:
                    localNode = localNode->parent;
                    continue;
                
                case nt_assignment_expression:
                    if (localNode == localNode->parent->u.assignment_expression.lvalue){
#ifdef DEBUGMAGICNUMBERS
                    string.append(QString("Assignment is: %1\n").arg(printer.toString(localNode->parent, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */
                        findAssignment = false;
                        localNode = localNode->parent;
                    }else return;
                    break;

                case nt_multiplicative_expression:
                    //Expression is part of calculation
                case nt_builtin_function_sizeof:
                case nt_builtin_function_offsetof:
                    return;
                
                case nt_unary_expression_op:
                    while(localNode->parent){
                        if (localNode->parent->type == nt_postfix_expression_parens){
                            member->evaluateMagicNumberFoundNotConstant();
                            return;
                        }
                        localNode = localNode->parent;
                    }
                    return;

                case nt_unary_expression_dec:
                case nt_unary_expression_inc:
                    member->evaluateMagicNumberFoundNotConstant();
                    return;
                
                default:
#ifdef DEBUGMAGICNUMBERS
                    string.append(QString("Unknown ASTType while going upwards: %1\n").arg(ast_node_type_to_str(localNode)));
                    string.append(QString("Node was: %1\n").arg(printer.toString(localNode, false).trimmed()));
                    debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                    localNode = localNode->parent;
                    return;
            }
        }
        
#ifdef DEBUGMAGICNUMBERS
        string.append(QString("Final Node Found was: %1\n").arg(printer.toString(localNode, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */
        
        if (antlrTokenToStr(localNode->u.assignment_expression.assignment_operator) != "=")
        {
            if(member) member->evaluateMagicNumberFoundNotConstant();
            return;
        }

        if (member && 
            !member->hasNotConstValue() &&
            localNode->u.assignment_expression.assignment_expression && 
            localNode->u.assignment_expression.assignment_expression->u.assignment_expression.conditional_expression &&
            localNode->u.assignment_expression.assignment_expression->u.assignment_expression.conditional_expression
                ->u.conditional_expression.logical_or_expression->u.binary_expression.left)
        {
            bool intConst = false;
            qint64 resultInt = 0;
            bool stringConst = false;
            QString resultString;

            evaluateMagicNumbers_constant(localNode->u.assignment_expression.assignment_expression
                    ->u.assignment_expression.conditional_expression
                    ->u.conditional_expression.logical_or_expression,
                    &intConst, &resultInt, 
                    &stringConst, &resultString,
                    string);

            if ((!intConst && !stringConst)){
                member->evaluateMagicNumberFoundNotConstant();
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Found not constant\n"));
#endif /* DEBUGMAGICNUMBERS */
            } else if(intConst){
                member->evaluateMagicNumberFoundInt(resultInt);
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Found constant Int: %1\n").arg(resultInt));
#endif /* DEBUGMAGICNUMBERS */
            } else if (stringConst) {
                member->evaluateMagicNumberFoundString(resultString);
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Found constant String: %1\n").arg(resultString));
#endif /* DEBUGMAGICNUMBERS */
            }
        }
    }


#ifdef DEBUGMAGICNUMBERS
    //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
}
