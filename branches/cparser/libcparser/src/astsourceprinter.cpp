/*
 * astsourceprinter.cpp
 *
 *  Created on: 18.04.2011
 *      Author: chrschn
 */

#include <astsourceprinter.h>
#include <abstractsyntaxtree.h>
#include <astsymbol.h>
#include <QTextDocument>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <debug.h>


ASTSourcePrinter::ASTSourcePrinter(AbstractSyntaxTree* ast)
    : ASTWalker(ast), _indent(0)
{
}


ASTSourcePrinter::~ASTSourcePrinter()
{
}


inline QString ASTSourcePrinter::tokenToString(pANTLR3_COMMON_TOKEN token)
{
    return antlrTokenToStr(token);
}


inline QString ASTSourcePrinter::tokenListToString(pASTTokenList list,
        const QString& delim)
{
    QString ret;

    for (pASTTokenList p = list; p; p = p->next) {
        ret += tokenToString(p->item);
        if (p->next && !delim.isEmpty())
            ret += delim;
    }

    return ret;
}

inline void ASTSourcePrinter::newlineIncIndent()
{
    ++_indent;
    newlineIndent();
}


inline void ASTSourcePrinter::newlineDecIndent()
{
    --_indent;
    newlineIndent();
}


inline void ASTSourcePrinter::newlineIndent()
{
    _out += "\n";
    for (int i = 0; i < _indent; ++i)
        _out += "\t";
}


void ASTSourcePrinter::beforeChildren(pASTNode node, int flags)
{
    Q_UNUSED(flags);

    switch (node->type) {
    case nt_abstract_declarator_suffix_brackets:
        _out += "[";
        break;
    case nt_abstract_declarator_suffix_parens:
        _out += "(";
        break;
//    case nt_assembler_parameter:
//        if (node->u.assembler_parameter.alias) {
//            _out += " [";
//            _out += tokenToString(node->u.assembler_parameter.alias);
//            _out += "]";
//        }
//        if (node->u.assembler_parameter.constraint_list)
//            _out += " " +tokenListToString(node->u.assembler_parameter.constraint_list, " ");
//        _out += " (";
//        break;
//    case nt_assembler_statement:
//        _out += "asm";
//        if (node->u.assembler_statement.type_qualifier_list)
//            _out += " " + tokenListToString(node->u.assembler_statement.type_qualifier_list, " ");
//        _out += " (";
//        _out += tokenListToString(node->u.assembler_statement.instructions, " ");
//        break;
    case nt_assignment_expression:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += ", ";
        break;
    case nt_builtin_function_alignof:
        _out += "__alignof__";
        if (node->u.builtin_function_alignof.type_name)
            _out += "(";
        break;
    case nt_builtin_function_choose_expr:
        _out += "__builtin_choose_expr(";
        break;
    case nt_builtin_function_constant_p:
        _out += "__builtin_constant_p";
        break;
    case nt_builtin_function_expect:
        _out += "__builtin_expect(";
        break;
    case nt_builtin_function_extract_return_addr:
        _out += "__builtin_extract_return_addr(";
        break;
    case nt_builtin_function_object_size:
        _out += "__builtin_object_size(";
        break;
    case nt_builtin_function_offsetof:
        _out += "__builtin_offsetof(";
        break;
    case nt_builtin_function_prefetch:
        _out += "__builtin_prefetch("
        break;
    case nt_builtin_function_return_address:
        _out += "__builtin_return_address(";
        break;
    case nt_builtin_function_sizeof:
        _out += "sizeof";
        if (node->u.builtin_function_sizeof.type_name)
            _out += "(";
        break;
    case nt_builtin_function_types_compatible_p:
        _out += "__builtin_types_compatible_p(";
        break;
    case nt_builtin_function_va_arg:
        _out += "__builtin_va_arg(";
        break;
    case nt_builtin_function_va_copy:
        _out += "__builtin_va_copy(";
        break;
    case nt_builtin_function_va_end:
        _out += "__builtin_va_end(";
        break;
    case nt_builtin_function_va_start:
        _out += "__builtin_va_start(";
        break;
    case nt_compound_braces_statement:
        _out += "({";
        newlineIncIndent();
        break;
    case nt_compound_statement:
        _out += "{";
        newlineIncIndent();
        break;
    case nt_constant_char:
    case nt_constant_float:
    case nt_constant_int:
        _out += tokenToString(node->u.constant.literal);
        break;
    case nt_constant_string:
        _out += tokenListToString(node->u.constant.string_token_list, " ");
        break;
    case nt_declaration:
        if (node->u.declaration.isTypedef)
            _out += "typedef ";
        break;
    case nt_declarator:
        if (node->u.declarator.isFuncDeclarator)
            _out += "(";
        break;
    case nt_declarator_suffix_brackets:
        _out += "[";
        break;
    case nt_declarator_suffix_parens:
        _out += "(";
        _out += tokenListToString(node->u.declarator_suffix.identifier_list, ",");
        break;
    case nt_designated_initializer:
        _out += "[";
        break;
    case nt_direct_declarator:
        _out += tokenToString(node->u.direct_declarator.identifier);
        break;
    case nt_enum_specifier:
        _out += "enum ";
        if (node->u.enum_specifier.identifier)
            _out += tokenToString(node->u.enum_specifier.identifier) + " ";
        break;
    case nt_enumerator:
        if ((flags & wfIsList) && !(flags & wfFirstInList)) {
            _out += ",";
            newlineIndent();
        }
        _out += tokenToString(node->u.enumerator.identifier);
        break;
    case nt_init_declarator:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += ", ";
        break;
    case nt_iteration_statement_do:
        _out += "do ";
        break;
    case nt_iteration_statement_for:
        _out += "for (";
        break;
    case nt_iteration_statement_while:
        _out += "while (";
        break;
//    case nt_jump_statement_break:
//        _out += "break";  /*, nodeId*/
//        _out += ";";  /*, nodeId*/
//        break;
//    case nt_jump_statement_continue:
//        _out += "continue";  /*, nodeId*/
//        _out += ";";  /*, nodeId*/
//        break;
//    case nt_jump_statement_goto:
//        _out += "goto";  /*, nodeId*/
//        break;
//    case nt_jump_statement_return:
//        _out += "return";  /*, nodeId*/
//        break;
    case nt_labeled_statement:
        _out += tokenToString(node->u.labeled_statement.identifier);
        _out += ":";
        newlineIndent();
        break;
    case nt_labeled_statement_case:
        _out += "case ";
        break;
    case nt_labeled_statement_default:
        _out += "default:";
        newlineIncIndent();
        break;
    case nt_parameter_declaration:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += ", ";
        break;
    case nt_pointer:
        _out += " *";
        if (node->u.pointer.type_qualifier_list)
            _out += tokenListToString(node->u.pointer.type_qualifier_list, " ") + " ";
        break;
    case nt_postfix_expression_brackets:
        _out += "[";
        break;
    case nt_postfix_expression_parens:
        _out += "(";
        break;
    case nt_postfix_expression_dot:
        _out += ".";
        _out += tokenToString(node->u.postfix_expression_suffix.identifier);
        break;
    case nt_postfix_expression_arrow:
        _out += "->";
        _out += tokenToString(node->u.postfix_expression_suffix.identifier);
        break;
    case nt_postfix_expression_inc:
        _out += "++";
        break;
    case nt_postfix_expression_dec:
        _out += "--";
        break;
    case nt_primary_expression:
        if (node->u.primary_expression.expression)
            _out += "(";
        if (node->u.primary_expression.hasDot)
            _out += ".";
        if (node->u.primary_expression.identifier)
            _out += tokenToString(node->u.primary_expression.identifier);
        break;
    case nt_selection_statement_if:
        _out += "if (";
        break;
    case nt_selection_statement_switch:
        _out += "switch (";
        break;
    case nt_storage_class_specifier:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += ", ";
        _out += tokenToString(node->u.storage_class_specifier.token);
        break;
    case nt_struct_declarator:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += ", ";
        break;
    case nt_struct_or_union_struct:
        _out += "struct ";
        break;
    case nt_struct_or_union_union:
        _out += "union ";
        break;
    case nt_type_id:
        _out += tokenToString(node->u.type_id.identifier);
        break;
    case nt_type_qualifier:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += " ";
        _out += tokenToString(node->u.type_qualifier.token);
        break;
    case nt_type_specifier:
        if (node->u.type_specifier.builtin_type_list)
            _out += tokenListToString(node->u.type_specifier.builtin_type_list, " ") + " ";
        else if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += " ";
        break;
    case nt_typeof_specification:
        _out += "typeof(";
        break;
    case nt_unary_expression_inc:
        _out += "++";
        break;
    case nt_unary_expression_dec:
        _out += "--";
        break;
    default:
        break;

    }
}


void ASTSourcePrinter::beforeChild(pASTNode node, pASTNode childNode)
{
    // Do we have a terminal token?
    switch (node->type) {
    case nt_abstract_declarator:
        if (childNode->type == nt_direct_abstract_declarator &&
            node->u.abstract_declarator.pointer)
            _out +=  " ";
    case nt_additive_expression:
    case nt_and_expression:
        if (node->u.binary_expression.right == childNode)
            _out += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_assignment_expression:
        if (childNode->type == nt_assignment_expression ||
            childNode->type == nt_initializer)
            _out += " " + tokenToString(node->parent->u.assignment_expression.assignment_operator) + " ";
        break;
    case nt_builtin_function_choose_expr:
        if (childNode->type == nt_assignment_expression)
            _out += ", ";
        break;
    case nt_builtin_function_offsetof:
        if (childNode->type == nt_postfix_expression)
            _out += ", ";
        break;
    case nt_builtin_function_prefetch:
        if (childNode->type == nt_constant_expression)
            _out += ", ";
        break;
    case nt_builtin_function_types_compatible_p:
        if (node-u.builtin_function_types_compatible_p.type_name2 == childNode)
            _out += ", ";
        break;
    case nt_builtin_function_va_arg:
        if (childNode->type == nt_type_name)
            _out += ", ";
        break;
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_start:
        if (node->u.builtin_function_va_xxx.assignment_expression2 == childNode)
            _out += ", ";
        break;
        break;

    case nt_cast_expression:
        if (childNode->type == nt_type_name)
            _out += "(";
        break;
    case nt_conditional_expression:
        if (childNode->type == nt_conditional_expression)
            _out += " : ";
        break;
    case nt_declaration:
        if (childNode->type == nt_init_declarator)
            _out += " ";
        break;
    case nt_designated_initializer:
        if (node->u.designated_initializer.constant_expression2 == childNode)
            _out += " ... ";
        break;
    case nt_direct_abstract_declarator:
        if (childNode->type == nt_abstract_declarator)
            _out += "(";
        break;
    case nt_enum_specifier:
        if (childNode->type == nt_enumerator) {
            _out += "{";
            newlineIncIndent();
        }
        break;
    case nt_enumerator:
        if (childNode->type == nt_constant_expression)
            _out += " = ";
        break;
    case nt_equality_expression:
    case nt_exclusive_or_expression:
    case nt_inclusive_or_expression:
        if (node->u.binary_expression.right == childNode)
            _out += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_init_declarator:
        if (childNode == node->u.init_declarator.initializer)
            _out += " = ";
        break;
    case nt_initializer:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _out += ", ";
        if (childNode->type == nt_initializer) {
            _out += "{";
            newlineIncIndent();
        }
        break;
    case nt_iteration_statement_do:
        if (node->u.iteration_statement.expression == childNode) {
            _out += ");";
            newlineIndent();
        }
        break;
    case nt_iteration_statement_for:
        _out += "for (";
        break;
    case nt_iteration_statement_while:
        _out += "while (";
    case nt_logical_and_expression:
    case nt_logical_or_expression:
        if (node->u.binary_expression.right == childNode)
            _out += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_lvalue:
        if (childNode->type == nt_type_name)
            _out += "(";
        break;
    case nt_multiplicative_expression:
        if (node->u.binary_expression.right == childNode)
            _out += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_parameter_declaration:
        if (childNode->type == nt_declarator || childNode->type == nt_abstract_declarator)
            _out += " ";
    case nt_relational_expression:
    case nt_shift_expression:
        if (node->u.binary_expression.right == childNode)
            _out += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_selection_statement_if:
        if (childNode == node->u.selection_statement.statement)
            _out += ")";
        if (childNode == node->u.selection_statement.statement_else)
            _out += "else";
        if (childNode->type == nt_compound_statement)
            _out += " ";
        else
            newlineIncIndent();
        break;
    case nt_struct_declaration:
        if (childNode->type == nt_struct_declarator)
            _out += " ";
        break;
    case nt_struct_declarator:
        if (childNode->type == nt_constant_expression)
            _out += ": "
        break;
    case nt_type_name:
        if (childNode->type == nt_abstract_declarator)
            _out += " ";
        break;
    case nt_unary_expression:
        if (childNode->type == nt_cast_expression)
            _out += tokenToString(node->u.unary_expression.unary_operator);
        break;
    default:
        break;
    }
}


void ASTSourcePrinter::afterChild(pASTNode node, pASTNode childNode)
{
    switch (node->type) {
    case nt_builtin_function_expect:
    case nt_builtin_function_object_size:
        if (childNode->type == nt_assignment_expression)
            _out += ", ";
        break;
    case nt_cast_expression:
        if (childNode->type == nt_type_name)
            _out += ") ";
        break;
    case nt_conditional_expression:
        if (childNode->type == nt_logical_or_expression &&
            node->u.conditional_expression.conditional_expression)
            _out += " ? ";
        break;
    case nt_declarator:
        if (childNode->type == nt_pointer)
            _out += " ";
    case nt_direct_abstract_declarator:
        if (childNode->type == nt_abstract_declarator)
            _out += ")";
        break;
    case nt_enumerator:
        if (flags & wfFirstInList)
            _out += "{";  /*, getNodeId(node->parent)*/
        _out += tokenToString(node->u.enumerator.identifier /*, nodeId*/);
        break;
    case nt_enum_specifier:
        if (childNode->type == nt_enumerator) {
            newlineDecIndent();
            _out += "}";
        }
        break;
    case nt_initializer:
        if (childNode->type == nt_initializer) {
            newlineDecIndent();
            _out += "}";
        }
        break;
    case nt_labeled_statement_case:
        if (childNode->type == nt_constant_expression) {
            if (childNode == node->u.labeled_statement.constant_expression &&
                node->u.labeled_statement.constant_expression2)
                _out += " ... ";
            else  {
                _out += ":";
                newlineIncIndent();
            }
        }
        break;
    case nt_lvalue:
        if (childNode->type == nt_type_name)
            _out += ") ";
        break;
    case nt_selection_statement_if:
        if (childNode->type != nt_compound_statement)
            newlineDecIndent();
        break;
    case nt_selection_statement_switch:
        if (childNode->type == nt_assignment_expression)
            _out += ") ";
        break;
    case nt_struct_or_union_specifier:
        if (childNode == node->u.struct_or_union_specifier.struct_or_union) {
            if (node->u.struct_or_union_specifier.identifier)
                _out += tokenToString(node->u.struct_or_union_specifier.identifier) + " ";
            if (node->u.struct_or_union_specifier.isDefinition) {
                _out += "{";
                newlineIncIndent();
            }
        }
        break;
    default:
        break;

    }
}

void ASTSourcePrinter::afterChildren(pASTNode node, int flags)
{
    Q_UNUSED(flags);
    QString nodeId = QString::number((quint64)node, 16);

    // Do we have a terminal token?
    switch (node->type) {
    case nt_abstract_declarator_suffix_brackets:
        _out += "]";
        break;
    case nt_abstract_declarator_suffix_parens:
        _out += ")";
        break;
//    case nt_assembler_parameter:
//        _out += ")";  /*, nodeId*/
//        if ((flags & wfIsList) && !(flags & wfLastInList))
//            _out += ",";  /*, getNodeId(node->parent)*/
//        break;
//    case nt_assembler_statement:
//        _out += ")";  /*, nodeId*/
//        _out += ";";  /*, nodeId*/
//        break;
    case nt_builtin_function_alignof:
        if (node->u.builtin_function_sizeof.type_name)
            _out += ")";
        break;
    case nt_builtin_function_constant_p:
        break;
    case nt_builtin_function_choose_expr:
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
        _out += ")";
        break;
    case nt_builtin_function_sizeof:
        if (node->u.builtin_function_alignof.type_name)
            _out += ")";
        break;
    case nt_compound_braces_statement:
        newlineDecIndent();
        _out += "})";
        break;
    case nt_compound_statement:
        newlineDecIndent();
        _out += "}";
        newlineIndent();
        break;
    case nt_declaration:
        _out += ";";
        newlineIndent();
        break;
    case nt_declarator:
        if (node->u.declarator.isFuncDeclarator)
            _out += ")";
        break;
    case nt_declarator_suffix_brackets:
        _out += "]";
        break;
    case nt_declarator_suffix_parens:
        _out += ")";
        break;
    case nt_designated_initializer:
        _out += "]";
        break;
    case nt_expression_statement:
        _out += ";";
        newlineIndent();
        break;
//    case nt_external_declaration:
//        if (node->u.external_declaration.assembler_statement)
//            _out += ";";  /*, nodeId*/
//        break;
//    case nt_init_declarator:
//        if ((flags & wfIsList) && !(flags & wfLastInList))
//            _out += ",";  /*, getNodeId(node->parent)*/
//        break;
//    case nt_initializer:
//        if (node->u.initializer.initializer_list)
//            _out += "}";  /*, nodeId*/
//        if ((flags & wfIsList) && !(flags & wfLastInList))
//            _out += ",";  /*, getNodeId(node->parent)*/
//        break;
//    case nt_iteration_statement_do:
//        _out += ")";  /*, nodeId*/
//        _out += ";";  /*, nodeId*/
//        break;
//    case nt_jump_statement_goto:
//    case nt_jump_statement_return:
//        _out += ";";  /*, nodeId*/
//        break;
    case nt_labeled_statement_case:
    case nt_labeled_statement_default:
        --_indent;
        break;
    case nt_parameter_type_list:
        if (node->u.parameter_type_list.openArgs)
            _out += ", ...";
        break;
    case nt_postfix_expression_brackets:
        _out += "]";
        break;
    case nt_postfix_expression_parens:
        _out += ")";
        break;
    case nt_primary_expression:
        if (node->u.primary_expression.expression)
            _out += ")";
        break;
    case nt_struct_declaration:
        _out += ";";
        newlineIndent();
        break;
    case nt_struct_or_union_specifier:
        if (node->u.struct_or_union_specifier.isDefinition) {
            newlineDecIndent();
            _out += "}";
        }
    case nt_typeof_specification:
        _out += ")";
        break;
    default:
        break;
   }
}


QString ASTSourcePrinter::toString()
{
    _out.clear();
    _indent = 0;
    if (_ast)
        walkTree();

    return _out;
}


QString ASTSourcePrinter::toString(pASTNode node)
{
    _out.clear();
    _indent = 0;
    if (_ast && node)
        walkTree(node);

    return _out;
}
