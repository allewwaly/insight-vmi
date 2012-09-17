/*
 * kernelsourcetypeevaluator.cpp
 *
 *  Created on: 05.09.2011
 *      Author: chrschn
 */

#include "kernelsourcetypeevaluator.h"
#include "symfactory.h"
#include <debug.h>
#include <bugreport.h>
#include <astnode.h>
#include <astscopemanager.h>
#include <astsourceprinter.h>
#include <abstractsyntaxtree.h>
#include "shell.h"
#include "astexpressionevaluator.h"
#include "expressionevalexception.h"

#define typeEvaluatorError(x) do { throw SourceTypeEvaluatorException((x), __FILE__, __LINE__); } while (0)


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
    return shell->interrupted();
}


int KernelSourceTypeEvaluator::evaluateIntExpression(const ASTNode* node, bool* ok)
{
    if (ok)
        *ok = false;

    if (node) {
        ASTExpression* expr = _eval->exprOfNode(node, ASTNodeNodeHash());
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
            typeEvaluatorError(
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

#define DEBUGMAGICNUMBERS   1


void KernelSourceTypeEvaluator::evaluateMagicNumbers_constant(const ASTNode *node, 
            bool *intConst, qint64 *resultInt, 
            bool *stringConst, QString *resultString,
            QString *string)
{
    ASTSourcePrinter printer(_ast);

    *intConst = false;
    *stringConst = false;

    switch(node->type){
    
        case nt_unary_expression_op:
        case nt_unary_expression_builtin: //TODO Maybe implement?
        case nt_builtin_function_sizeof:
        case nt_unary_expression_inc:
        case nt_unary_expression_dec:
            return;
        
        case nt_conditional_expression:
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
                string->append(QString("Stringconstant with operation?\n"));
                string->append(QString("Node was: %1\n").arg(printer.toString(node, false).trimmed()));
                debugmsg(*string);
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
                        string->append(QString("Type represents no binary expression\n"));
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
                    ASTType* constantType = typeofNode(node);
                    if(constantType->type() == rtArray && 
                            constantType->next() &&
                            constantType->next()->type() == rtInt8)
                    {
                        //Interesting as this could be names of modules/structs
#ifdef DEBUGMAGICNUMBERS
                        string->append(QString("Found string constant!! \"%1\"").arg(constant));
                        *stringConst = true;
#endif /* DEBUGMAGICNUMBERS */
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
                        string->append(QString("Found int32 constant: %1\n").arg(*resultInt));
#endif /* DEBUGMAGICNUMBERS */
                    }
                    //TODO also parse (u)int[16|64] (LL/ULL)
#ifdef DEBUGMAGICNUMBERS
                    else
                    {
                        RealType type = constantType->type();
                        string->append(QString("Found constant!! %1 : %2 : %3.")
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
            string->append(QString("Unknown ASTType while going downwards: %1\n").arg(ast_node_type_to_str(node)));
            string->append(QString("Node was: %1\n").arg(printer.toString(node, false).trimmed()));
            debugmsg(*string);
#endif /* DEBUGMAGICNUMBERS */
            return;
    }
}

void KernelSourceTypeEvaluator::evaluateMagicNumbers(const ASTNode *node)
{
    if(!node) return;

    ASTSourcePrinter printer(_ast);
    //Search for type // struct
    StructuredMember* member = 0;


#ifdef DEBUGMAGICNUMBERS
    QString string;
#endif /* DEBUGMAGICNUMBERS */

    if (node->type == nt_primary_expression &&
        node->parent &&
        node->parent->type == nt_postfix_expression)
    {
#ifdef DEBUGMAGICNUMBERS
        string.append(QString("\nCurrent Node: %1 \n")
                .arg(printer.toString(node, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */

        ASTType* constantType = typeofNode(node);

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

            const Structured* structured = 0;
            const ASTNodeList* list;

            if (!constantType->identifier().isEmpty())
            {
                QString memberName;

                //Find member in primaryNode to identify structured member
                //Handle Expression suffixes
                if (node->parent->u.postfix_expression.postfix_expression_suffix_list)
                {

                    list = node->parent->u.postfix_expression.postfix_expression_suffix_list;
                    while(list){
                        if (list && list->next && list->item->type == nt_postfix_expression_parens)
                            list = list->next;
                        else if (list && list->next && list->item->type == nt_postfix_expression_brackets)
                            list = list->next;
                        else if (list &&
                                (list->item->type == nt_postfix_expression_arrow ||
                                 list->item->type == nt_postfix_expression_dot) && 
                                !antlrTokenToStr(list->item->u.postfix_expression_suffix.identifier).isEmpty())
                        {
                            memberName = antlrTokenToStr(list->item->u.postfix_expression_suffix.identifier);
#ifdef DEBUGMAGICNUMBERS
                            string.append(QString("Found Membername: %1\n")
                                    .arg(memberName.trimmed()));
                            if (printer.toString(node, false).contains("orig_video_isVGA")) debugmsg(string); 
#endif /* DEBUGMAGICNUMBERS */
                            if(!structured){
                                BaseTypeList baseTypes = _factory->findBaseTypesByName(constantType->identifier());
                                BaseTypeList::iterator i;

                                for (i = baseTypes.begin(); i != baseTypes.end(); ++i)
                                {
                                    if((*i)->type() == rtStruct || (*i)->type() ==rtUnion)
                                    {
                                        structured = dynamic_cast<const Structured*>(*i);
                                        member = (StructuredMember*) structured->findMember(memberName);
                                        if (member){
                                            if (member->refType()->type() == rtStruct || member->refType()->type() ==rtUnion)
                                                structured = dynamic_cast<const Structured*>(member->refType());
                                            break;
                                        }
                                    }
                                }
                            } else {
                                member = (StructuredMember*) structured->findMember(memberName);
                                if (member){
                                    if (member->refType()->type() == rtStruct || member->refType()->type() ==rtUnion)
                                        structured = dynamic_cast<const Structured*>(member->refType());
                                }
                            }
                            if (list->next)
                                list = list->next;
                            else
                                break;
                        }else break;
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
                if (!structured)
                {
#ifdef DEBUGMAGICNUMBERS
                    string.append(QString("UNKNOWN STRUCT: %1 with member %2 - BUG?")
                            .arg(constantType->identifier())
                            .arg(memberName)
                            );
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

#ifdef DEBUGMAGICNUMBERS
                if (member->refType()->type() == typeofNode(list->item)->type() || 
                        ((member->refType()->type() == rtTypedef ||
                          member->refType()->type() == rtVolatile) &&
                         member->refType()->dereferencedBaseType()->type() == typeofNode(list->item)->type())
                   )
                {
                    string.append(QString("Found Assignment: \"%1->(%2)\" (%3)\n")
                            .arg(structured->name())
                            .arg(member->prettyName())
                            .arg(printer.toString(node, false).trimmed())
                            );
                }
#endif /* DEBUGMAGICNUMBERS */
            }
            else
            {
                //Struct got no identifier, so we do not know the type
#ifdef DEBUGMAGICNUMBERS
                string.append(QString("Struct without name?"));
                // debugmsg(string);
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

        const ASTNode* localNode = node->parent;
        bool findAssignment = true;
        while (findAssignment && localNode->parent)
        {
            switch(localNode->type)
            {
                case nt_postfix_expression:
                case nt_unary_expression:
                case nt_cast_expression:
                case nt_lvalue:
                    localNode = localNode->parent;
                    continue;
                
                case nt_assignment_expression:
                    if (localNode == localNode->parent->u.assignment_expression.lvalue)
                    string.append(QString("Assignment is: %1\n").arg(printer.toString(localNode, false).trimmed()));
                    findAssignment = false;
                    break;

                case nt_multiplicative_expression:
                    //Expression is part of calculation
                case nt_builtin_function_sizeof:
                case nt_builtin_function_offsetof:
                case nt_unary_expression_op:
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
            member->evaluateMagicNumberFoundNotConstant();
            return;
        }

        //bool constantValue = false;
        //int constantInt = evaluateIntExpression(localNode->u.assignment_expression.assignment_expression, &constantValue);

        //if(constantValue)
        //{
        //    member->evaluateMagicNumberFoundInt(constantInt);
#ifdef DEBUGMAGICNUMBERS
        //    string.append(QString("Found constant: %1\n").arg(constantInt));
        //    return;
#endif /* DEBUGMAGICNUMBERS */
        //}
        //else
        //{
        //    //Try to find String Value!
        //    member->evaluateMagicNumberFoundNotConstant();
        //    return;
        //}

        if (localNode->u.assignment_expression.assignment_expression && 
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
                    &string);

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



#ifdef DEBUGMAGICNUMBERS
        //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
    }

    else if(false && node->type == nt_assignment_expression)
    {

        ASTNode* localNode;
        if (node->u.assignment_expression.assignment_expression && 
                node->u.assignment_expression.assignment_expression->u.assignment_expression.conditional_expression &&
                node->u.assignment_expression.assignment_expression->u.assignment_expression.conditional_expression
                ->u.conditional_expression.logical_or_expression->u.binary_expression.left)
        {

#ifdef DEBUGMAGICNUMBERS
            string.append(QString("\nCurrent Node: %1 \n")
                    .arg(printer.toString(node, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */

            //Search for type // struct
            StructuredMember* member = 0;

            if (node->u.assignment_expression.lvalue)
            {
                localNode = node->u.assignment_expression.lvalue;
#ifdef DEBUGMAGICNUMBERS
            string.append(QString("\nCurrent Node: %1 \n")
                    .arg(printer.toString(localNode, false).trimmed()));
#endif /* DEBUGMAGICNUMBERS */
                if (localNode->u.lvalue.unary_expression)
                {
                    localNode = localNode->u.lvalue.unary_expression;

                    //If node is a cast, find corresponding object
                    while (localNode->u.unary_expression.cast_expression)
                    {
                        if (localNode->u.unary_expression.cast_expression->u.cast_expression.cast_expression)
                        {
                            localNode = localNode->u.unary_expression.cast_expression;
                            while (localNode->u.cast_expression.cast_expression)
                            {
                                localNode = localNode->u.cast_expression.cast_expression;
                            }
                            if (localNode->u.cast_expression.unary_expression)
                                localNode = localNode->u.cast_expression.unary_expression;
                            else return;
                        }
                        else if (localNode->u.unary_expression.cast_expression->u.cast_expression.unary_expression)
                            localNode = localNode->u.unary_expression.cast_expression->u.cast_expression.unary_expression;
                    }

                    if (localNode->u.unary_expression.unary_expression)
                        return;
                    
                    if (localNode->u.unary_expression.postfix_expression)
                    {
                        localNode = localNode->u.unary_expression.postfix_expression;

                        if (localNode->u.postfix_expression.primary_expression)
                        {
                            ASTNode* primaryNode = localNode->u.postfix_expression.primary_expression;
                            ASTType* constantType = typeofNode(primaryNode);
                            
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

                                const Structured* structured = 0;
                                const ASTNodeList* list;

                                if (!constantType->identifier().isEmpty())
                                {
                                    QString memberName;
                                    
                                    //Find member in primaryNode to identify structured member
                                    //Handle Expression suffixes
                                    if (localNode->u.postfix_expression.postfix_expression_suffix_list)
                                    {

                                        list = localNode->u.postfix_expression.postfix_expression_suffix_list;
                                        while(list){
                                            if (list && list->next && list->item->type == nt_postfix_expression_parens)
                                                list = list->next;
                                            else if (list && list->next && list->item->type == nt_postfix_expression_brackets)
                                                list = list->next;
                                            else if (list &&
                                                    (list->item->type == nt_postfix_expression_arrow ||
                                                     list->item->type == nt_postfix_expression_dot) && 
                                                    !antlrTokenToStr(list->item->u.postfix_expression_suffix.identifier).isEmpty())
                                            {
                                                memberName = antlrTokenToStr(list->item->u.postfix_expression_suffix.identifier);
#ifdef DEBUGMAGICNUMBERS
                                                string.append(QString("Found Membername: %1\n")
                                                    .arg(memberName.trimmed()));
                                                if (printer.toString(node, false).contains("orig_video_isVGA")) debugmsg(string); 
#endif /* DEBUGMAGICNUMBERS */
                                                if(!structured){
                                                    BaseTypeList baseTypes = _factory->findBaseTypesByName(constantType->identifier());
                                                    BaseTypeList::iterator i;
                                                    
                                                    for (i = baseTypes.begin(); i != baseTypes.end(); ++i)
                                                    {
                                                        if((*i)->type() == rtStruct || (*i)->type() ==rtUnion)
                                                        {
                                                            structured = dynamic_cast<const Structured*>(*i);
                                                            member = (StructuredMember*) structured->findMember(memberName);
                                                            if (member){
                                                                if (member->refType()->type() == rtStruct || member->refType()->type() ==rtUnion)
                                                                  structured = dynamic_cast<const Structured*>(member->refType());
                                                                break;
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    member = (StructuredMember*) structured->findMember(memberName);
                                                    if (member){
                                                        if (member->refType()->type() == rtStruct || member->refType()->type() ==rtUnion)
                                                            structured = dynamic_cast<const Structured*>(member->refType());
                                                    }
                                                }
                                                if (list->next)
                                                    list = list->next;
                                                else
                                                    break;
                                            }else break;
                                        }
                                    }
                                    else
                                    {
#ifdef DEBUGMAGICNUMBERS
                                            string.append(QString(
                                                        "Struct without member?\n")
                                                    );
#endif /* DEBUGMAGICNUMBERS */
                                    }
                                    if (structured)
                                    {
#ifdef DEBUGMAGICNUMBERS
                                        string.append(QString("Found corresponding Basetype: %1 Type: %2\n")
                                                .arg(structured->name())
                                                .arg(realTypeToStr(structured->type()))
                                                );
#endif /* DEBUGMAGICNUMBERS */
                                    }
                                    else
                                    {
#ifdef DEBUGMAGICNUMBERS
                                        string.append(QString("UNKNOWN STRUCT: %1 with member %2 - BUG?")
                                                .arg(constantType->identifier())
                                                .arg(memberName)
                                                );
#endif /* DEBUGMAGICNUMBERS */
                                        //TODO ask Christian
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


                                    if (member->refType()->type() == typeofNode(list->item)->type() || 
                                            ((member->refType()->type() == rtTypedef ||
                                              member->refType()->type() == rtVolatile) &&
                                             member->refType()->dereferencedBaseType()->type() == typeofNode(list->item)->type())
                                       )
                                    {
#ifdef DEBUGMAGICNUMBERS
                                        string.append(QString("Found Assignment: \"%1->(%2)\" (%3)\n")
                                                .arg(structured->name())
                                                .arg(member->prettyName())
                                                .arg(printer.toString(node, false).trimmed())
                                                );
#endif /* DEBUGMAGICNUMBERS */
                                    }
#ifdef DEBUGMAGICNUMBERS
                                    else
                                    {
                                        string.append(QString("Found Assignment with different type: \"%1->(%2)\" (%4) Types: %5<>%6\n")
                                                .arg(structured->name())
                                                .arg(member->prettyName())
                                                .arg(printer.toString(node, false).trimmed())
                                                .arg(realTypeToStr(member->refType()->type()))
                                                .arg(realTypeToStr(typeofNode(list->item)->type()))
                                                );
                                    }
#endif /* DEBUGMAGICNUMBERS */
                                }
                                else
                                {
                                    //Struct got no identifier, so we do not know the type
                                    //Maybe ask Christian
#ifdef DEBUGMAGICNUMBERS
                                    string.append(QString("Struct without name?"));
                                   // debugmsg(string);
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

                        }
#ifdef DEBUGMAGICNUMBERS
                        else
                        {
                            string.append(QString(
                                        "\nPostfixNode: %1"
                                        "\n\tpirmary_expression: %2"
                                        "\n\tpostfix_expression_suffix_list: %3\n")
                                    .arg(printer.toString(localNode,false)) 
                                    .arg(printer.toString(localNode->u.postfix_expression.primary_expression,false)) 
                                    .arg((localNode->u.postfix_expression.postfix_expression_suffix_list) ? "List" : "") 
                                    );
                        }
#endif /* DEBUGMAGICNUMBERS */
                    }
#ifdef DEBUGMAGICNUMBERS
                    else
                    {
                        string.append(QString(
                                    "\nUnaryNode: %1"
                                    "\n\tunary_expression: %2"
                                    "\n\tcast_expression: %3"
                                    "\n\tpostfix_expression: %4"
                                    "\n\tbuiltin_function: %5\n")
                                .arg(printer.toString(localNode,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.unary_expression,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.cast_expression,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.postfix_expression,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.builtin_function,false)) 
                                ); 
                        if (localNode->u.unary_expression.cast_expression)
                        {
                            localNode = localNode->u.unary_expression.cast_expression;
                            string.append(QString(
                                        "\nCastNode: %1"
                                        "\n\ttype_name: %2"
                                        "\n\tcast_expression: %3"
                                        "\n\tinitializer: %4"
                                        "\n\tunary_expression: %5"
                                        "\n\tunary_expression type: %6\n")
                                    .arg(printer.toString(localNode->u.unary_expression.cast_expression,false)) 
                                    .arg(printer.toString(localNode->u.unary_expression.cast_expression->u.cast_expression.type_name,false)) 
                                    .arg(printer.toString(localNode->u.unary_expression.cast_expression->u.cast_expression.cast_expression,false)) 
                                    .arg(printer.toString(localNode->u.unary_expression.cast_expression->u.cast_expression.initializer,false)) 
                                    .arg(printer.toString(localNode->u.unary_expression.cast_expression->u.cast_expression.unary_expression), false)
                                    .arg(ast_node_type_to_str(localNode->u.unary_expression.cast_expression->u.cast_expression.unary_expression))
                                    ); 
                        }
                    }
#endif /* DEBUGMAGICNUMBERS */
                }
#ifdef DEBUGMAGICNUMBERS
                else
                {
                    string.append(QString(
                                "\nLvalue: %1"
                                "\n\ttype_name: %2"
                                "\n\tlvalue: %3"
                                "\n\tunary_expression: %4\n")
                            .arg(printer.toString(localNode,false)) 
                            .arg(printer.toString(localNode->u.lvalue.type_name,false)) 
                            .arg(printer.toString(localNode->u.lvalue.lvalue,false)) 
                            .arg(printer.toString(localNode->u.lvalue.unary_expression,false)) 
                            );
                }
#endif /* DEBUGMAGICNUMBERS */
            }
#ifdef DEBUGMAGICNUMBERS
            else
            {
                string.append(QString(
                            "\nAssignmentExpression: %1"
                            "\n\tlvalue: %2"
                            "\n\tassignmentExpression: %3"
                            "\n\tdesignated_initializer_list: %4"
                            "\n\tinitializer: %5"
                            "\n\tconditional_expression: %6\n")
                        .arg(printer.toString(node,false)) 
                        .arg(printer.toString(node->u.assignment_expression.lvalue,false)) 
                        .arg(printer.toString(node->u.assignment_expression.assignment_expression,false)) 
                        .arg((node->u.assignment_expression.designated_initializer_list) ? "LIST" : "") //printer.toString(localNode->u.primary_expression.expression,false)
                        .arg(printer.toString(node->u.assignment_expression.initializer,false)) 
                        .arg(printer.toString(node->u.assignment_expression.conditional_expression,false)) 
                        ); 
            }
#endif /* DEBUGMAGICNUMBERS */
            

            if (antlrTokenToStr(node->u.assignment_expression.assignment_operator) != "=")
            {
                member->evaluateMagicNumberFoundNotConstant();
                return;
            }

            //Search for constant
                
            qint64 constantInt = 0;
            bool foundConstant = false;

            //Search for constant in assignment_expression
            int32_t mode = 0;
            localNode = node->u.assignment_expression.assignment_expression
                ->u.assignment_expression.conditional_expression
                ->u.conditional_expression.logical_or_expression;
            while(mode >= 0)
            {
                if(mode == 0)
                {
#ifdef DEBUGMAGICNUMBERS
                    if(localNode->u.binary_expression.left)
                        string.append(QString("left type %1 (%2) %3 - ")
                                .arg(ast_node_type_to_str(localNode->u.binary_expression.left))
                                .arg(localNode->u.binary_expression.left->type)
                                .arg(printer.toString(localNode->u.binary_expression.left))
                                );
#endif /* DEBUGMAGICNUMBERS */
                    if ( localNode->u.binary_expression.right)
                    {
                        //TODO handle composed statements (could also be constant!)
#ifdef DEBUGMAGICNUMBERS
                        string.append("composed type, skipping....\n");
#endif /* DEBUGMAGICNUMBERS */
                        mode = -1;
                    }
                    localNode = localNode->u.binary_expression.left;
                    if(localNode->type == 24) //Cast_expression
                        mode = 1;
                }
                else if (mode == 1)
                {
                    if (localNode->u.cast_expression.unary_expression)
                    {
                        localNode = localNode->u.cast_expression.unary_expression;
                        mode = 2;
                    }
                    else
                    {
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString(
                                    "\nCastNode: %1"
                                    "\n\ttype_name: %2"
                                    "\n\tcast_expression: %3"
                                    "\n\tinitializer: %4"
                                    "\n\tunary_expression: %5"
                                    "\n\tunary_expression type: %6")
                                .arg(printer.toString(localNode,false)) 
                                .arg(printer.toString(localNode->u.cast_expression.type_name,false)) 
                                .arg(printer.toString(localNode->u.cast_expression.cast_expression,false)) 
                                .arg(printer.toString(localNode->u.cast_expression.initializer,false)) 
                                .arg(printer.toString(localNode->u.cast_expression.unary_expression), false)
                                .arg(ast_node_type_to_str(localNode->u.cast_expression.unary_expression))
                                ); 
                        //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                        mode = -1;
                    }
                }
                else if (mode == 2)
                {
                    if (localNode->u.unary_expression.postfix_expression)
                    {
                        localNode = localNode->u.unary_expression.postfix_expression;
                        mode = 3;
                    }
                    else
                    {
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString(
                                    "\nUnaryNode: %1"
                                    "\n\tunary_expression: %2"
                                    "\n\tcast_expression: %3"
                                    "\n\tpostfix_expression: %4"
                                    "\n\tbuiltin_function: %5")
                                .arg(printer.toString(localNode,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.unary_expression,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.cast_expression,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.postfix_expression,false)) 
                                .arg(printer.toString(localNode->u.unary_expression.builtin_function,false)) 
                                ); 
                        //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                        mode = -1;
                    }
                }
                else if (mode == 3)
                {
                    if (localNode->u.postfix_expression.primary_expression)
                    {
                        localNode = localNode->u.postfix_expression.primary_expression;
                        mode = 4;
                    }
                    else
                    {
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString(
                                    "\nPostfixNode: %1"
                                    "\n\tpirmary_expression: %2"
                                    "\n\tpostfix_expression_suffix_list: %3")
                                .arg(printer.toString(localNode,false)) 
                                .arg(printer.toString(localNode->u.postfix_expression.primary_expression,false)) 
                                .arg((localNode->u.postfix_expression.postfix_expression_suffix_list) ? "List" : "") 
                                ); 
                        //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                        mode = -1;
                    }
                }
                else if (mode == 4)
                {
                    if (localNode->u.primary_expression.constant)
                    {
                        QString constant = printer.toString(localNode->u.primary_expression.constant,false).trimmed();
#ifdef DEBUGMAGICNUMBERS
                        QString buffer;
                        //debugmsg(QString("Found ConstantExpression: %1 : %2 : %3")
                        //         .arg(printer.toString(node,false))
                        //         .arg(constant)
                        //         .arg(constant.length())
                        //         );
#endif /* DEBUGMAGICNUMBERS */
                        if (!constant.isNull())
                        {
                            ASTType* constantType = typeofNode(localNode);
#ifdef DEBUGMAGICNUMBERS
                            buffer.append(QString("Found Constant: %1 = %2, Assignment was: %3 : Type: %4")
                                    //.arg(findSymbolOfPrimaryExpression(node)->name())
                                    .arg(constant)
                                    .arg(printer.toString(node,false))
                                    .arg(constantType->toString())
                                    );
#endif /* DEBUGMAGICNUMBERS */
                            if(constantType->type() == rtArray && 
                                    constantType->next() &&
                                    constantType->next()->type() == rtInt8)
                            {
                                //Found string constant
                                //Interesting as this could be names of modules/structs
#ifdef DEBUGMAGICNUMBERS
                                buffer.append(QString("Found string constant!!"));
#endif /* DEBUGMAGICNUMBERS */
                                //Assignment found TODO handle it!
                                if (!member->evaluateMagicNumberFoundString(constant))
                                {
                                    _factory->seenMagicNumbers.append(member);
                                }
                                return;
                            }
                            else if (constantType->type() == rtInt32)
                            {
                                //Try to parse integer as it could be interesting
                                //Cut off L
                                if(constant.indexOf("L", 0, Qt::CaseInsensitive) > -1)
                                    constant.truncate(constant.indexOf("U", 0, Qt::CaseInsensitive));

                                constantInt = constant.toInt( &foundConstant, 0 );

#ifdef DEBUGMAGICNUMBERS
                                buffer.append(QString("Fount int32 constant: %1").arg(constantInt));
#endif /* DEBUGMAGICNUMBERS */
                            }
                            else if (constantType->type() == rtUInt32)
                            {
                                //Try to parse integer as it could be interesting
                                //Cut off U/UL/ULL
                                if(constant.indexOf("U", 0, Qt::CaseInsensitive) > -1)
                                    constant.truncate(constant.indexOf("U", 0, Qt::CaseInsensitive));
                                if(constant.indexOf("L", 0, Qt::CaseInsensitive) > -1)
                                    constant.truncate(constant.indexOf("U", 0, Qt::CaseInsensitive));

                                constantInt = constant.toInt( &foundConstant, 0 );

#ifdef DEBUGMAGICNUMBERS
                                buffer.append(QString("Fount uint32 constant: %1").arg(constantInt));
#endif /* DEBUGMAGICNUMBERS */
                            }
                            //TODO also parse (u)int[16|64] (LL/ULL)
#ifdef DEBUGMAGICNUMBERS
                            else
                            {
                                RealType type = constantType->type();
                                buffer.append(QString("Found constant!! %1 : %2 : %3.")
                                        .arg(realTypeToStr(type))
                                        .arg(constantType->identifier())
                                        .arg((constantType->next()) ? realTypeToStr(constantType->next()->type()): "?")
                                        );
                            }
                            buffer.append(string);
                            //debugmsg(buffer);
#endif /* DEBUGMAGICNUMBERS */
                        }
                        //localNode = localNode->u.primary_expression.constant;
                        mode = 5;
                        mode = -1;
                    }
                    else
                    {
#ifdef DEBUGMAGICNUMBERS
                        string.append(QString(
                                    "\nPrimaryExpression: %1"
                                    "\n\tconstant: %2"
                                    "\n\texpression: %3"
                                    "\n\tcompound_braces_statement: %4")
                                .arg(printer.toString(localNode,false)) 
                                .arg(printer.toString(localNode->u.primary_expression.constant,false)) 
                                .arg((localNode->u.primary_expression.expression) ? "LIST" : "") //printer.toString(localNode->u.primary_expression.expression,false)
                                .arg(printer.toString(localNode->u.primary_expression.compound_braces_statement,false)) 
                                ); 
                        //debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                        member->evaluateMagicNumberFoundNotConstant();
                        mode = -1;
                    }
                }
                if(!localNode)
                {
#ifdef DEBUGMAGICNUMBERS
                    debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */
                    mode = -1;
                }
            }

#ifdef DEBUGMAGICNUMBERS
//                    debugmsg(string);
#endif /* DEBUGMAGICNUMBERS */

            //Assignment found TODO handle it!
            if (foundConstant && !member->evaluateMagicNumberFoundInt(constantInt))
            {
                _factory->seenMagicNumbers.append(member);
            }
            return;
        }
    }
}
