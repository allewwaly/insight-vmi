/*
 * astdotgraph.cpp
 *
 *  Created on: 18.04.2011
 *      Author: chrschn
 */

#include <astdotgraph.h>
#include <abstractsyntaxtree.h>
#include <astsymbol.h>
#include <asttypeevaluator.h>

#include <QTextDocument>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <debug.h>

#define FONT_SIZE "10.0"

#define FONT_SIZE_LINE " POINT-SIZE=\"8\" "
#define FONT_COLOR_LINE " COLOR=\"#aaaaaa\" "
#define FONT_DEF_LINE FONT_SIZE_LINE FONT_COLOR_LINE

#define FONT_FACE_STR " FACE=\"Courier\" "
#define FONT_DEF_STR FONT_FACE_STR

#define NODE_NEW_SCOPE_BG      "fillcolor=\"#FFFF99\""
#define TOK_PRIM_EX_IDENTIFIER "fillcolor=\"#99FF99\""
#define TOK_POSTFIX_EX         "fillcolor=\"#AACCAA\""
#define TOK_DIR_DECL           "fillcolor=\"#FF9999\""
#define TOK_TYPE_SPECIFIER     "fillcolor=\"#AAAACC\""


ASTDotGraph::ASTDotGraph(ASTTypeEvaluator *eval)
    : ASTWalker(eval ? eval->ast() : 0), _eval(eval)
{
}


ASTDotGraph::ASTDotGraph(AbstractSyntaxTree* ast)
    : ASTWalker(ast), _eval(0)
{
}


ASTDotGraph::~ASTDotGraph()
{
}


inline QString ASTDotGraph::dotEscape(const QString& s) const
{
    return Qt::escape(s).replace('[', "&#91;").replace(']', "&#93;");
}


inline QString ASTDotGraph::getNodeId(const ASTNode *node) const
{
    return QString::number((quint64)node, 16);
}


inline QString ASTDotGraph::getTokenId(pANTLR3_COMMON_TOKEN token) const
{
    return QString::number((quint64)token, 16);
}


inline void ASTDotGraph::printDotGraphNodeLabel(const ASTNode *node)
{
    if (!node)
        return;

    QString line = node->start ? QString::number(node->start->line) : "?";
    QString pos = node->start ? QString::number(node->start->charPosition) : "?";

    // Node's own label
    _out << QString("\t// %1, line %2:%3, ID %4")
            .arg(ast_node_type_to_str(node))
            .arg(line)
            .arg(pos)
            .arg(getNodeId(node))
        << endl;

    // Print nodes with own scope in different color
    QString fill;
    if (node->scope != node->childrenScope)
        fill = ",style=filled," NODE_NEW_SCOPE_BG;

    _out << QString("\tnode_%1 [shape=box,label=< %2 <FONT " FONT_DEF_LINE ">%3:%4</FONT> >%5];")
            .arg(getNodeId(node))
            .arg(ast_node_type_to_str(node))
            .arg(line)
            .arg(pos)
            .arg(fill)
        << endl;
}


inline void ASTDotGraph::printDotGraphTokenLabel(pANTLR3_COMMON_TOKEN token,
                                                 const char* extraStyle)
{
    if (!token)
        return;

    _out << QString("\ttoken_%1 [label=< <FONT " FONT_DEF_STR ">%2</FONT> >,style=filled%3];")
            .arg(getTokenId(token))
            .arg(dotEscape((const char*)token->getText(token)->chars))
            .arg(extraStyle ? QString(",%1").arg(extraStyle) : QString())
        << endl;
}


void ASTDotGraph::printDotGraphToken(pANTLR3_COMMON_TOKEN token,
        const QString& parentNodeId, const char* extraStyle)
{
    if (!token)
        return;

    printDotGraphTokenLabel(token, extraStyle);
    _out << QString("\t\tnode_%1 -> token_%2;")
            .arg(parentNodeId)
            .arg(getTokenId(token))
        << endl;
}


void ASTDotGraph::printDotGraphString(const QString& s,
        const QString& parentNodeId, const char* extraStyle)
{
    static int stringId = 0;
    int id = stringId++;

    _out << QString("\tstring_%1 [label=< <FONT " FONT_DEF_STR ">%2</FONT> >,style=filled%3];")
            .arg(id)
            .arg(dotEscape(s))
            .arg(extraStyle ? QString(",%1").arg(extraStyle) : QString())
        << endl;
    _out << QString("\t\tnode_%1 -> string_%2;")
            .arg(parentNodeId)
            .arg(id)
        << endl;
}


void ASTDotGraph::printDotGraphTokenList(pASTTokenList list,
        const QString& delim, const QString& nodeId, const char* extraStyle)
{
    if (!list)
        return;

    for (pASTTokenList p = list; p; p = p->next) {
        printDotGraphToken(p->item, nodeId, extraStyle);
        if (p->next && !delim.isEmpty())
            printDotGraphString(delim, nodeId);
    }
}


void ASTDotGraph::printDotGraphConnection(pANTLR3_COMMON_TOKEN src,
                                          const ASTNode* dest, int derefCount)
{
    QString srcId = getTokenId(src);
    QString destId = getNodeId(dest);
    QString label;
    if (derefCount) {
        if (derefCount < 0)
            label.fill(QChar('&'), -derefCount);
        else
            label.fill(QChar('*'), derefCount);

        label = QString(" taillabel=< <FONT " FONT_DEF_STR ">%1</FONT> > "
                        "labelfloat=true labelangle=-45 labeldistance=2")
                    .arg(label);
    }
    _out << QString("\t\ttoken_%1 -> node_%2 [constraint=false style=dotted "
                    "layer=\"assign\"%3];")
            .arg(srcId)
            .arg(destId)
            .arg(label)
         << endl;
}


void ASTDotGraph::beforeChildren(const ASTNode* node, int flags)
{
    Q_UNUSED(flags);

    // Nodes we are not interested in
    switch (node->type) {
    case nt_struct_or_union_struct:
    case nt_struct_or_union_union:
        return;
    default:
    	break;
    }

    QString nodeId = getNodeId(node);

    // Do we have a terminal token?
    switch (node->type) {
    case nt_abstract_declarator_suffix_brackets:
        printDotGraphString("[", nodeId);
        break;
    case nt_abstract_declarator_suffix_parens:
        printDotGraphString("(", nodeId);
        break;
    case nt_additive_expression:
        if (node->parent->type == nt_shift_expression &&
                    node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_and_expression:
        if (node->parent->type == nt_exclusive_or_expression &&
                    node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_assembler_parameter:
        if ((flags & wfIsList) && (flags & wfFirstInList))
            printDotGraphString(":", getNodeId(node->parent));

        if (node->u.assembler_parameter.alias) {
            printDotGraphString("[", nodeId);
            printDotGraphToken(node->u.assembler_parameter.alias, nodeId);
            printDotGraphString("]", nodeId);
        }
        printDotGraphTokenList(node->u.assembler_parameter.constraint_list, "", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_assembler_statement:
        printDotGraphString("asm", nodeId);
        printDotGraphTokenList(node->u.assembler_statement.type_qualifier_list, "", nodeId);
        printDotGraphString("(", nodeId);
        printDotGraphTokenList(node->u.assembler_statement.instructions, "", nodeId);
        break;
    case nt_assignment_expression:
        if (node->parent->type == nt_assignment_expression)
            printDotGraphToken(node->parent->u.assignment_expression.assignment_operator, getNodeId(node->parent));
        else if (node->parent->type == nt_builtin_function_choose_expr)
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_builtin_function_alignof:
        printDotGraphString("__alignof__", nodeId);
        if (node->u.builtin_function_alignof.type_name)
            printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_choose_expr:
        printDotGraphString("__builtin_choose_expr", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_constant_p:
        printDotGraphString("__builtin_constant_p", nodeId);
        break;
    case nt_builtin_function_expect:
        printDotGraphString("__builtin_expect", nodeId);
		printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_extract_return_addr:
        printDotGraphString("__builtin_extract_return_addr", nodeId);
		printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_object_size:
        printDotGraphString("__builtin_object_size", nodeId);
		printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_offsetof:
        printDotGraphString("__builtin_offsetof", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_prefetch:
        printDotGraphString("__builtin_prefetch", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_return_address:
        printDotGraphString("__builtin_return_address", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_sizeof:
        printDotGraphString("sizeof", nodeId);
        if (node->u.builtin_function_sizeof.type_name)
            printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_types_compatible_p:
        printDotGraphString("__builtin_types_compatible_p", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_va_arg:
        printDotGraphString("__builtin_va_arg", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_va_copy:
        printDotGraphString("__builtin_va_copy", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_va_end:
        printDotGraphString("__builtin_va_end", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_builtin_function_va_start:
        printDotGraphString("__builtin_va_start", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_cast_expression:
        if (node->parent->type == nt_multiplicative_expression &&
                node->parent->u.binary_expression.right == node)
        printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        if (node->u.cast_expression.type_name)
            printDotGraphString("(", nodeId);
        break;
    case nt_compound_braces_statement:
        printDotGraphString("({", nodeId);
        break;
    case nt_compound_statement:
        printDotGraphString("{", nodeId);
        break;
    case nt_conditional_expression:
        if (node->parent->type == nt_conditional_expression)
            printDotGraphString(":", getNodeId(node->parent));
        break;
    case nt_constant_char:
    case nt_constant_float:
    case nt_constant_int:
        printDotGraphToken(node->u.constant.literal, nodeId);
        break;
    case nt_constant_string:
        printDotGraphTokenList(node->u.constant.string_token_list, "", nodeId);
        break;
    case nt_constant_expression:
        if (node->parent->type == nt_struct_declarator)
            printDotGraphString(":", getNodeId(node->parent));
        else if (node->parent->type == nt_enumerator)
            printDotGraphString("=", getNodeId(node->parent));
        else if (node->parent->type == nt_designated_initializer) {
            if (node->parent->u.designated_initializer.constant_expression1 == node)
                printDotGraphString("[", getNodeId(node->parent));
            else
                printDotGraphString("...", getNodeId(node->parent));
        }
        else if (node->parent->type == nt_builtin_function_prefetch)
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_declaration:
        if (node->u.declaration.isTypedef)
            printDotGraphString("typedef", nodeId);
        break;
    case nt_declarator_suffix_brackets:
        printDotGraphString("[", nodeId);
        break;
    case nt_declarator_suffix_parens:
        printDotGraphString("(", nodeId);
        printDotGraphTokenList(node->u.declarator_suffix.identifier_list, ",", nodeId);
        break;
    case nt_designated_initializer:
        break;
    case nt_direct_abstract_declarator:
        break;
    case nt_direct_declarator:
        printDotGraphToken(node->u.direct_declarator.identifier, nodeId,
                           TOK_DIR_DECL);
        if (node->u.direct_declarator.declarator) {
            printDotGraphString("(", nodeId);
        }
//        if (node->u.direct_declarator.identifier) {
//            const ASTSymbol* sym = _eval->findSymbolOfDirectDeclarator(node);
//            if (sym && !sym->assignedAstNodes().isEmpty()) {
//                for (int i = 0; i < sym->assignedAstNodes().size(); ++i)
//                    printDotGraphConnection(
//                                node->u.direct_declarator.identifier,
//                                sym->assignedAstNodes().at(i));
//            }
//        }
        break;
    case nt_enum_specifier:
        printDotGraphString("enum", nodeId, TOK_TYPE_SPECIFIER);
        printDotGraphToken(node->u.enum_specifier.identifier, nodeId, TOK_TYPE_SPECIFIER);
        break;
    case nt_enumerator:
        if (flags & wfFirstInList)
            printDotGraphString("{", getNodeId(node->parent));
        printDotGraphToken(node->u.enumerator.identifier, nodeId);
        break;
    case nt_equality_expression:
        if (node->parent->type == nt_and_expression &&
                    node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_exclusive_or_expression:
        if (node->parent->type == nt_inclusive_or_expression &&
                    node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_inclusive_or_expression:
        if (node->parent->type == nt_logical_and_expression &&
                node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_initializer:
        // Print an equals sign, if we are part of an init_declarator
        if (node->parent->type == nt_init_declarator)
            printDotGraphString("=", getNodeId(node->parent));
        else if (node->parent->type == nt_assignment_expression)
            printDotGraphToken(node->parent->u.assignment_expression.assignment_operator, getNodeId(node->parent));
        if (node->u.initializer.initializer_list)
            printDotGraphString("{", nodeId);
        break;
    case nt_iteration_statement_do:
        printDotGraphString("do", nodeId);
        break;
    case nt_iteration_statement_for:
        printDotGraphString("for", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_iteration_statement_while:
        printDotGraphString("while", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_jump_statement_break:
        printDotGraphString("break", nodeId);
        printDotGraphString(";", nodeId);
        break;
    case nt_jump_statement_continue:
        printDotGraphString("continue", nodeId);
        printDotGraphString(";", nodeId);
        break;
    case nt_jump_statement_goto:
        printDotGraphString("goto", nodeId);
        break;
    case nt_jump_statement_return:
        printDotGraphString("return", nodeId);
        break;
    case nt_labeled_statement:
        printDotGraphToken(node->u.labeled_statement.identifier, nodeId);
        printDotGraphString(":", nodeId);
        break;
    case nt_labeled_statement_case:
        printDotGraphString("case", nodeId);
        break;
    case nt_labeled_statement_default:
        printDotGraphString("default", nodeId);
        printDotGraphString(":", nodeId);
        break;
    case nt_logical_and_expression:
        if (node->parent->type == nt_logical_or_expression &&
                node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_lvalue:
        if (node->parent->type == nt_lvalue)
            printDotGraphString(")", getNodeId(node->parent));
        if (node->u.lvalue.type_name)
            printDotGraphString("(", nodeId);
        break;
    case nt_multiplicative_expression:
        if (node->parent->type == nt_additive_expression &&
                node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_pointer: {
        // Is this within a declaration?
        const ASTNode* p = node->parent;
        while (p && p->type != nt_declarator)
            p = p->parent;
        if (p)
            printDotGraphString("*", nodeId, TOK_DIR_DECL);
        else
            printDotGraphString("*", nodeId);
        printDotGraphTokenList(node->u.pointer.type_qualifier_list, "", nodeId);
    }
        break;
    case nt_postfix_expression:
        break;
    case nt_postfix_expression_brackets:
        printDotGraphString("[", nodeId);
        break;
    case nt_postfix_expression_parens:
        printDotGraphString("(", nodeId);
        break;
    case nt_postfix_expression_dot:
        printDotGraphString(".", nodeId, TOK_POSTFIX_EX);
        printDotGraphToken(node->u.postfix_expression_suffix.identifier, nodeId, TOK_POSTFIX_EX);
        break;
    case nt_postfix_expression_arrow:
        printDotGraphString("->", nodeId, TOK_POSTFIX_EX);
        printDotGraphToken(node->u.postfix_expression_suffix.identifier, nodeId, TOK_POSTFIX_EX);
        break;
    case nt_postfix_expression_inc:
        printDotGraphString("++", nodeId);
        break;
    case nt_postfix_expression_dec:
        printDotGraphString("--", nodeId);
        break;
    case nt_primary_expression:
        if (node->u.primary_expression.expression)
            printDotGraphString("(", nodeId);
        if (node->u.primary_expression.hasDot)
            printDotGraphString(".", nodeId);
        printDotGraphToken(node->u.primary_expression.identifier, nodeId, TOK_PRIM_EX_IDENTIFIER);
        if (_eval && node->u.primary_expression.identifier) {
            const ASTSymbol* sym = _eval->findSymbolOfPrimaryExpression(node);
            if (sym && !sym->assignedAstNodes().isEmpty()) {
                for (AssignedNodeSet::const_iterator it =
                        sym->assignedAstNodes().begin(),
                     e = sym->assignedAstNodes().end(); it != e; ++it)
                {
                    printDotGraphConnection(
                                node->u.primary_expression.identifier,
                                it->node,
                                it->derefCount);
                }
            }
        }

        break;
    case nt_relational_expression:
        if (node->parent->type == nt_equality_expression &&
                    node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_shift_expression:
        if (node->parent->type == nt_relational_expression &&
                    node->parent->u.binary_expression.right == node)
            printDotGraphToken(node->parent->u.binary_expression.op, getNodeId(node->parent));
        break;
    case nt_selection_statement_if:
        printDotGraphString("if", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_selection_statement_switch:
        printDotGraphString("switch", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_storage_class_specifier:
        printDotGraphToken(node->u.storage_class_specifier.token, nodeId);
        break;
    case nt_struct_declaration:
        if (flags & wfFirstInList)
            printDotGraphString("{", getNodeId(node->parent));
        break;
    case nt_struct_or_union_specifier:
        if (node->u.struct_or_union_specifier.struct_or_union) {
            const ASTNode* sou = node->u.struct_or_union_specifier.struct_or_union;
            if (sou->type == nt_struct_or_union_struct)
                printDotGraphString("struct", nodeId, TOK_TYPE_SPECIFIER);
            else
                printDotGraphString("union", nodeId, TOK_TYPE_SPECIFIER);
        }
        printDotGraphToken(node->u.struct_or_union_specifier.identifier, nodeId, TOK_TYPE_SPECIFIER);
        break;
    case nt_type_id:
        printDotGraphToken(node->u.type_id.identifier, nodeId, TOK_TYPE_SPECIFIER);
        break;
    case nt_type_qualifier:
        printDotGraphToken(node->u.type_qualifier.token, nodeId);
        break;
    case nt_type_specifier:
        printDotGraphTokenList(node->u.type_specifier.builtin_type_list, QString(), nodeId, TOK_TYPE_SPECIFIER);
        break;
    case nt_typeof_specification:
        printDotGraphString("typeof", nodeId);
        printDotGraphString("(", nodeId);
        break;
    case nt_unary_expression:
        printDotGraphToken(node->u.unary_expression.unary_operator, nodeId);
        break;
    case nt_unary_expression_inc:
        printDotGraphString("++", nodeId);
        break;
    case nt_unary_expression_dec:
        printDotGraphString("--", nodeId);
        break;
    case nt_unary_expression_op:
        // Specially handle the fake "&&" unary operator
        if ("&&" == antlrTokenToStr(node->u.unary_expression.unary_operator))
            printDotGraphString("&", nodeId);
        else
            printDotGraphToken(node->u.unary_expression.unary_operator, nodeId);
        break;
    default:
    	break;

    }

    // Special case: statements
    switch (node->type) {
    case nt_labeled_statement:
    case nt_compound_statement:
//    case nt_compound_braces_statement:
    case nt_expression_statement:
    case nt_selection_statement_if:
    case nt_selection_statement_switch:
    case nt_iteration_statement_do:
    case nt_iteration_statement_while:
    case nt_iteration_statement_for:
    case nt_jump_statement_break:
    case nt_jump_statement_continue:
    case nt_jump_statement_goto:
    case nt_jump_statement_return:
    case nt_assembler_statement:
        if (node->parent->type == nt_selection_statement_if) {
            if (node->parent->u.selection_statement.statement == node)
                printDotGraphString(")", getNodeId(node->parent));
            else
                printDotGraphString("else", getNodeId(node->parent));
        }
        else if (node->parent->type == nt_selection_statement_switch) {
            printDotGraphString(")", getNodeId(node->parent));
        }
        else if (node->parent->type == nt_iteration_statement_while &&
                node == node->parent->u.iteration_statement.statement)
        {
            printDotGraphString(")", getNodeId(node->parent));
        }
        else if (node->parent->type == nt_iteration_statement_for &&
                node == node->parent->u.iteration_statement.statement) {
            printDotGraphString(")", getNodeId(node->parent));
        }
        break;
    default:
    	break;

    }

    // Node's own label
    printDotGraphNodeLabel(node);
    // Connection to parent
    if (node->parent) {
        _out << QString("\t\tnode_%1 -> node_%2;")
                .arg(getNodeId(node->parent))
                .arg(nodeId)
            << endl;
    }
}


void ASTDotGraph::afterChildren(const ASTNode* node, int flags)
{
    Q_UNUSED(flags);
    QString nodeId = getNodeId(node);

    // Do we have a terminal token?
    switch (node->type) {
    case nt_abstract_declarator_suffix_brackets:
        printDotGraphString("]", nodeId);
        break;
    case nt_abstract_declarator_suffix_parens:
        printDotGraphString(")", nodeId);
        break;
    case nt_assembler_parameter:
        printDotGraphString(")", nodeId);
        if ((flags & wfIsList) && !(flags & wfLastInList))
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_assembler_statement:
        printDotGraphString(")", nodeId);
        printDotGraphString(";", nodeId);
        break;
    case nt_assignment_expression:
        if ((flags & wfIsList) && !(flags & wfLastInList))
            printDotGraphString(",", getNodeId(node->parent));
        else if (node->parent->type == nt_builtin_function_expect ||
        		node->parent->type == nt_builtin_function_object_size ||
        		node->parent->type == nt_builtin_function_va_arg)
            printDotGraphString(",", getNodeId(node->parent));
        else if ((node->parent->type == nt_builtin_function_va_copy ||
        		 node->parent->type == nt_builtin_function_va_start) &&
        		 node == node->parent->u.builtin_function_va_xxx.assignment_expression)
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_builtin_function_alignof:
        if (node->u.builtin_function_sizeof.type_name)
            printDotGraphString(")", nodeId);
        break;
    case nt_builtin_function_choose_expr:
        printDotGraphString(")", nodeId);
        break;
    case nt_builtin_function_constant_p:
        break;
    case nt_builtin_function_expect:
    case nt_builtin_function_extract_return_addr:
    case nt_builtin_function_object_size:
    case nt_builtin_function_offsetof:
    case nt_builtin_function_prefetch:
    case nt_builtin_function_return_address:
    case nt_builtin_function_types_compatible_p:
    case nt_builtin_function_va_arg:
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_end:
    case nt_builtin_function_va_start:
        printDotGraphString(")", nodeId);
        break;
    case nt_builtin_function_sizeof:
        if (node->u.builtin_function_alignof.type_name)
            printDotGraphString(")", nodeId);
        break;
//    case nt_cast_expression:
//      if (node->u.cast_expression.type_name)
//          printDotGraphString(")", nodeId);
//        break;
    case nt_compound_braces_statement:
        printDotGraphString("})", nodeId);
        break;
    case nt_compound_statement:
        printDotGraphString("}", nodeId);
        break;
    case nt_constant_expression:
        if (node->parent->type == nt_labeled_statement_case) {
            if (!node->parent->u.labeled_statement.constant_expression2 ||
                    node == node->parent->u.labeled_statement.constant_expression2)
                printDotGraphString(":", getNodeId(node->parent));
            else
                printDotGraphString("...", getNodeId(node->parent));
        }
        else if (node->parent->type == nt_designated_initializer) {
            if (!node->parent->u.designated_initializer.constant_expression2 ||
                 node->parent->u.designated_initializer.constant_expression2 == node)
            printDotGraphString("]", getNodeId(node->parent));
        }
        break;
    case nt_declaration:
        printDotGraphString(";", nodeId);
        break;
    case nt_declarator:
        if (node->parent->type == nt_direct_declarator)
            printDotGraphString(")", getNodeId(node->parent));
        break;
    case nt_declarator_suffix_brackets:
        printDotGraphString("]", nodeId);
        break;
    case nt_declarator_suffix_parens:
        printDotGraphString(")", nodeId);
        break;
//    case nt_direct_abstract_declarator:
//        break;
//    case nt_direct_declarator:
//        if (node->u.direct_declarator.declarator)
//            printDotGraphString(")", nodeId);
//        break;
    case nt_enumerator:
        if (flags & wfLastInList)
            printDotGraphString("}", getNodeId(node->parent));
        else
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_expression_statement:
        printDotGraphString(";", nodeId);
        break;
    case nt_external_declaration:
        if (node->u.external_declaration.assembler_statement)
            printDotGraphString(";", nodeId);
        break;
    case nt_init_declarator:
        if ((flags & wfIsList) && !(flags & wfLastInList))
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_initializer:
        if (node->u.initializer.initializer_list)
            printDotGraphString("}", nodeId);
        if ((flags & wfIsList) && !(flags & wfLastInList))
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_iteration_statement_do:
        printDotGraphString(")", nodeId);
        printDotGraphString(";", nodeId);
        break;
    case nt_jump_statement_goto:
    case nt_jump_statement_return:
        printDotGraphString(";", nodeId);
        break;
    case nt_logical_or_expression:
        if (node->parent->type == nt_conditional_expression &&
                node->parent->u.conditional_expression.conditional_expression)
            printDotGraphString("?", getNodeId(node->parent));
        break;
    case nt_parameter_declaration:
        if ((flags & wfIsList) && !(flags & wfLastInList))
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_parameter_type_list:
        if (node->u.parameter_type_list.openArgs) {
            printDotGraphString(",", nodeId);
            printDotGraphString("...", nodeId);
        }
        break;
    case nt_postfix_expression_brackets:
        printDotGraphString("]", nodeId);
        break;
    case nt_postfix_expression_parens:
        printDotGraphString(")", nodeId);
        break;
    case nt_primary_expression:
        if (node->u.primary_expression.expression)
            printDotGraphString(")", nodeId);
        break;
    case nt_struct_declaration:
        printDotGraphString(";", nodeId);
        if (flags & wfLastInList)
            printDotGraphString("}", getNodeId(node->parent));
        break;
    case nt_struct_declarator:
        if ((flags & wfIsList) && !(flags & wfLastInList))
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_type_name:
        if (node->parent->type == nt_cast_expression)
            printDotGraphString(")", getNodeId(node->parent));
        else if (node->parent->type == nt_builtin_function_types_compatible_p &&
                 node->parent->u.builtin_function_types_compatible_p.type_name1 == node)
            printDotGraphString(",", getNodeId(node->parent));
        else if (node->parent->type == nt_builtin_function_offsetof)
            printDotGraphString(",", getNodeId(node->parent));
        break;
    case nt_typeof_specification:
        printDotGraphString(")", nodeId);
        break;
    default:
    	break;
   }

    // Special case: statements
    switch (node->type) {
    case nt_labeled_statement:
    case nt_compound_statement:
//    case nt_compound_braces_statement:
    case nt_expression_statement:
    case nt_selection_statement_if:
    case nt_selection_statement_switch:
    case nt_iteration_statement_do:
    case nt_iteration_statement_while:
    case nt_iteration_statement_for:
    case nt_jump_statement_break:
    case nt_jump_statement_continue:
    case nt_jump_statement_goto:
    case nt_jump_statement_return:
    case nt_assembler_statement:
        if (node->parent->type == nt_iteration_statement_do &&
                node->parent->u.iteration_statement.statement == node)
        {
            printDotGraphString("while", getNodeId(node->parent));
            printDotGraphString("(", getNodeId(node->parent));
        }
        break;
    default:
    	break;

    }
}


int ASTDotGraph::writeDotGraph(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        debugerr("Error opening file '" << fileName << "' for writing");
        return -1;
    }

    _out.setDevice(&file);
    int visits = 0;

    // Write header, data and footer
    _out << "/* Generated " << QDateTime::currentDateTime().toString() << " */"
            << endl
            << "digraph G {" << endl;
    _out << "\tgraph [ordering=out];" << endl;
    _out << "\tlayers = \"ast:assign\";" << endl;
    _out << "\tedge [fontname=Helvetica fontsize=" FONT_SIZE " layer=\"ast\"];" << endl;
    _out << "\tnode [fontname=Helvetica fontsize=" FONT_SIZE " layer=\"ast\"];" << endl
        << endl;

    if (_ast) {
        // Print label for root node
        _out << QString("\tnode_%1 [shape=box,style=filled,label=\"%2\"];")
                .arg("0")
                .arg("translation_unit")
            << endl;

        for (const ASTNodeList* p = _ast->rootNodes(); p; p = p->next) {
            if (!p->item)
                continue;
            _out << QString("\t\tnode_%1 -> node_%2;")
                    .arg("0")
                    .arg(getNodeId(p->item))
                << endl;
        }

        visits = walkTree();
    }

    _out << "}" << endl;

    _out.setDevice(0);
    file.close();

    return visits;
}


int ASTDotGraph::writeDotGraph(const ASTNode* node, const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
        debugerr("Error opening file '" << fileName << "' for writing");
        return -1;
    }

    _out.setDevice(&file);
    int visits = 0;

    // Write header, data and footer
    _out << "/* Generated " << QDateTime::currentDateTime().toString() << " */"
            << endl
            << "digraph G {" << endl;
    _out << "\tgraph [ordering=out];" << endl;
    _out << "\tnode [fontname=Helvetica fontsize=" FONT_SIZE "];" << endl
        << endl;

    if (node) {
        // Work our way up from node to the first parent
        const ASTNode *child = node, *par = node->parent;
        while (par) {
            // Parent's label
            printDotGraphNodeLabel(par);
            // Connection between child and parent
            if (node->parent) {
                _out << QString("\t\tnode_%1 -> node_%2;")
                        .arg(getNodeId(par))
                        .arg(getNodeId(child))
                    << endl;
            }

            child = par;
            par = par->parent;
            visits++;
        }

        // Print label for root node
        _out << QString("\tnode_%1 [shape=box,style=filled,label=\"%2\"];")
                .arg("0")
                .arg("translation_unit")
            << endl;
        // Connection to root node
        _out << QString("\t\tnode_%1 -> node_%2;")
                .arg("0")
                .arg(getNodeId(child))
            << endl;

        _stopWalking = false;
        visits += walkTree(node);
    }

    _out << "}" << endl;

    _out.setDevice(0);
    file.close();

    return visits;
}
