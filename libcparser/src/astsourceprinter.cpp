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
    : ASTWalker(ast), _indent(0), _lineIndent(0), _currNode(0)
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


inline void ASTSourcePrinter::newlineIndent(bool forceIfEmpty)
{
    if (forceIfEmpty || !_line.isEmpty())
        _out += lineIndent() + _line + "\n";
    _line.clear();
    _lineIndent = _indent;
}


inline QString ASTSourcePrinter::lineIndent() const
{
    QString ret;
    if (_prefixLineNo)
        ret = QString("%1: ").arg(_currNode->start->line);
    for (int i = 0; i < _lineIndent; ++i)
        ret += "    ";
    return ret;
}


void ASTSourcePrinter::beforeChildren(const ASTNode *node, int flags)
{
    Q_UNUSED(flags);    
    _currNode = node;

    switch (node->type) {
    case nt_abstract_declarator_suffix_brackets:
        _line += "[";
        break;
    case nt_abstract_declarator_suffix_parens:
        _line += "(";
        break;
    case nt_assembler_parameter:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += ", ";
        if (node->u.assembler_parameter.alias) {
            _line += " [";
            _line += tokenToString(node->u.assembler_parameter.alias);
            _line += "] ";
        }
        if (node->u.assembler_parameter.constraint_list)
            _line += tokenListToString(node->u.assembler_parameter.constraint_list, " ") + " ";
        _line += "(";
        break;
    case nt_assembler_statement:
        _line += "asm ";
        if (node->u.assembler_statement.type_qualifier_list)
            _line += " " + tokenListToString(node->u.assembler_statement.type_qualifier_list, " ") + " ";
        _line += " (";
        _line += tokenListToString(node->u.assembler_statement.instructions, " ");
        break;
    case nt_assignment_expression:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += ", ";
        break;
    case nt_builtin_function_alignof:
        _line += "__alignof__";
        if (node->u.builtin_function_alignof.type_name)
            _line += "(";
        break;
    case nt_builtin_function_choose_expr:
        _line += "__builtin_choose_expr(";
        break;
    case nt_builtin_function_constant_p:
        _line += "__builtin_constant_p";
        break;
    case nt_builtin_function_expect:
        _line += "__builtin_expect(";
        break;
    case nt_builtin_function_extract_return_addr:
        _line += "__builtin_extract_return_addr(";
        break;
    case nt_builtin_function_object_size:
        _line += "__builtin_object_size(";
        break;
    case nt_builtin_function_offsetof:
        _line += "__builtin_offsetof(";
        break;
    case nt_builtin_function_prefetch:
        _line += "__builtin_prefetch(";
        break;
    case nt_builtin_function_return_address:
        _line += "__builtin_return_address(";
        break;
    case nt_builtin_function_sizeof:
        _line += "sizeof";
        if (node->u.builtin_function_sizeof.type_name)
            _line += "(";
        break;
    case nt_builtin_function_types_compatible_p:
        _line += "__builtin_types_compatible_p(";
        break;
    case nt_builtin_function_va_arg:
        _line += "__builtin_va_arg(";
        break;
    case nt_builtin_function_va_copy:
        _line += "__builtin_va_copy(";
        break;
    case nt_builtin_function_va_end:
        _line += "__builtin_va_end(";
        break;
    case nt_builtin_function_va_start:
        _line += "__builtin_va_start(";
        break;
    case nt_compound_braces_statement:
        _line += "({";
        newlineIncIndent();
        break;
    case nt_compound_statement:
        _line += "{";
        newlineIncIndent();
        break;
    case nt_constant_char:
    case nt_constant_float:
    case nt_constant_int:
        _line += tokenToString(node->u.constant.literal);
        break;
    case nt_constant_string:
        _line += tokenListToString(node->u.constant.string_token_list, " ");
        break;
    case nt_declaration:
        if (node->u.declaration.isTypedef)
            _line += "typedef ";
        break;
    case nt_declarator:
        if (node->u.declarator.isFuncDeclarator)
            _line += "(";
        break;
    case nt_declarator_suffix_brackets:
        _line += "[";
        break;
    case nt_declarator_suffix_parens:
        _line += "(";
        _line += tokenListToString(node->u.declarator_suffix.identifier_list, ",");
        break;
    case nt_designated_initializer:
        if (node->u.designated_initializer.identifier)
            _line += "." + antlrTokenToStr(node->u.designated_initializer.identifier);
        else
            _line += "[";
        break;
    case nt_direct_declarator:
        _line += tokenToString(node->u.direct_declarator.identifier);
        break;
    case nt_enum_specifier:
        _line += "enum";
        if (node->u.enum_specifier.identifier)
            _line += " " + tokenToString(node->u.enum_specifier.identifier);
        break;
    case nt_enumerator:
        if ((flags & wfIsList) && !(flags & wfFirstInList)) {
            _line += ",";
            newlineIndent();
        }
        _line += tokenToString(node->u.enumerator.identifier);
        break;
    case nt_init_declarator:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += ", ";
        break;
    case nt_initializer:
        if ((flags & wfIsList) && !(flags & wfFirstInList)) {
            _line += ",";
            newlineIndent();
        }
        break;
    case nt_iteration_statement_do:
        _line += "do";
        break;
    case nt_iteration_statement_for:
        _line += "for (";
        break;
    case nt_iteration_statement_while:
        _line += "while (";
        break;
    case nt_jump_statement_break:
        _line += "break;";
        newlineIndent();
        break;
    case nt_jump_statement_continue:
        _line += "continue;";
        newlineIndent();
        break;
    case nt_jump_statement_goto:
        _line += "goto ";
        break;
    case nt_jump_statement_return:
        _line += "return";
        break;
    case nt_labeled_statement:
        _line += tokenToString(node->u.labeled_statement.identifier);
        _line += ":";
        newlineIndent();
        break;
    case nt_labeled_statement_case:
        _line += "case ";
        break;
    case nt_labeled_statement_default:
        _line += "default:";
        newlineIncIndent();
        break;
    case nt_parameter_declaration:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += ", ";
        break;
    case nt_pointer:
        _line += "*";
        if (node->u.pointer.type_qualifier_list)
            _line += " " + tokenListToString(node->u.pointer.type_qualifier_list, " ");
        break;
    case nt_postfix_expression_brackets:
        _line += "[";
        break;
    case nt_postfix_expression_parens:
        _line += "(";
        break;
    case nt_postfix_expression_dot:
        _line += ".";
        _line += tokenToString(node->u.postfix_expression_suffix.identifier);
        break;
    case nt_postfix_expression_arrow:
        _line += "->";
        _line += tokenToString(node->u.postfix_expression_suffix.identifier);
        break;
    case nt_postfix_expression_inc:
        _line += "++";
        break;
    case nt_postfix_expression_dec:
        _line += "--";
        break;
    case nt_primary_expression:
        if (node->u.primary_expression.expression)
            _line += "(";
        if (node->u.primary_expression.identifier)
            _line += tokenToString(node->u.primary_expression.identifier);
        break;
    case nt_selection_statement_if:
        _line += "if (";
        break;
    case nt_selection_statement_switch:
        _line += "switch (";
        break;
    case nt_storage_class_specifier:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += " ";
        _line += tokenToString(node->u.storage_class_specifier.token);
        break;
    case nt_struct_declarator:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += ", ";
        break;
    case nt_struct_or_union_struct:
        _line += "struct";
        break;
    case nt_struct_or_union_union:
        _line += "union";
        break;
    case nt_type_id:
        _line += tokenToString(node->u.type_id.identifier);
        break;
    case nt_type_qualifier:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += " ";
        _line += tokenToString(node->u.type_qualifier.token);
        break;
    case nt_type_specifier:
        if ((flags & wfIsList) && !(flags & wfFirstInList))
            _line += " ";
        if (node->u.type_specifier.builtin_type_list)
            _line += tokenListToString(node->u.type_specifier.builtin_type_list, " ");
        break;
    case nt_typeof_specification:
        _line += "typeof(";
        break;
    case nt_unary_expression_dec:
        _line += "--";
        break;
    case nt_unary_expression_inc:
        _line += "++";
        break;
    case nt_unary_expression_op:
        _line += tokenToString(node->u.unary_expression.unary_operator);
    default:
        break;
    }
}


void ASTSourcePrinter::beforeChild(const ASTNode *node, const ASTNode *childNode)
{
    switch (node->type) {
    case nt_abstract_declarator:
        if (childNode->type == nt_direct_abstract_declarator &&
            node->u.abstract_declarator.pointer)
            _line +=  " ";
        break;
    case nt_additive_expression:
    case nt_and_expression:
        if (node->u.binary_expression.right == childNode)
            _line += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_assembler_statement:
        if (childNode->type == nt_assembler_parameter)
            _line += " : ";
        break;
    case nt_assignment_expression:
        if (childNode->type == nt_assignment_expression ||
            childNode->type == nt_initializer)
            _line += " " + tokenToString(node->u.assignment_expression.assignment_operator) + " ";
        break;
    case nt_builtin_function_choose_expr:
        if (childNode->type == nt_assignment_expression)
            _line += ", ";
        break;
    case nt_builtin_function_offsetof:
        if (childNode->type == nt_postfix_expression)
            _line += ", ";
        break;
    case nt_builtin_function_prefetch:
        if (childNode->type == nt_constant_expression)
            _line += ", ";
        break;
    case nt_builtin_function_types_compatible_p:
        if (node->u.builtin_function_types_compatible_p.type_name2 == childNode)
            _line += ", ";
        break;
    case nt_builtin_function_va_arg:
        if (childNode->type == nt_type_name)
            _line += ", ";
        break;
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_start:
        if (node->u.builtin_function_va_xxx.assignment_expression2 == childNode)
            _line += ", ";
        break;
    case nt_cast_expression:
        if (childNode->type == nt_type_name)
            _line += "(";
        break;
    case nt_conditional_expression:
        if (childNode->type == nt_conditional_expression)
            _line += " : ";
        break;
    case nt_declaration:
        if (childNode->type == nt_init_declarator)
            _line += " ";
        break;
    case nt_designated_initializer:
        if (node->u.designated_initializer.constant_expression2 == childNode)
            _line += " ... ";
        break;
    case nt_direct_abstract_declarator:
        if (childNode->type == nt_abstract_declarator)
            _line += "(";
        break;
    case nt_enum_specifier:
        if (childNode->type == nt_enumerator) {
            _line += " {";
            newlineIncIndent();
        }
        break;
    case nt_enumerator:
        if (childNode->type == nt_constant_expression)
            _line += " = ";
        break;
    case nt_equality_expression:
    case nt_exclusive_or_expression:
        if (node->u.binary_expression.right == childNode)
            _line += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_function_definition:
        if (childNode->type == nt_declarator)
            _line += " ";
        else if (childNode->type == nt_compound_statement)
            newlineIndent();
        break;
    case nt_inclusive_or_expression:
        if (node->u.binary_expression.right == childNode)
            _line += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_init_declarator:
        if (childNode == node->u.init_declarator.initializer)
            _line += " = ";
        break;
    case nt_initializer:
        if (childNode->type == nt_initializer) {
            _line += "{";
            newlineIncIndent();
        }
        break;
    case nt_iteration_statement_do:
        if (node->u.iteration_statement.statement == childNode) {
            if (node->u.iteration_statement.statement->type == nt_compound_statement)
                _line += " ";
            else
                newlineIncIndent();
        }
        else {
            if (node->u.iteration_statement.statement->type == nt_compound_statement)
                newlineIndent();
            else
                newlineDecIndent();
            _line += "while (";
        }
        break;
    case nt_iteration_statement_for:
    case nt_iteration_statement_while:
        if (node->u.iteration_statement.statement == childNode) {
            _line += ")";
            if (childNode->type == nt_compound_statement)
                _line += " ";
            else
                newlineIncIndent();
        }
        break;
    case nt_jump_statement_return:
        if (childNode->type == nt_initializer)
            _line += " ";
        break;
    case nt_logical_and_expression:
    case nt_logical_or_expression:
        if (node->u.binary_expression.right == childNode)
            _line += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_lvalue:
        if (childNode->type == nt_type_name)
            _line += "(";
        break;
    case nt_multiplicative_expression:
        if (node->u.binary_expression.right == childNode)
            _line += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_parameter_declaration:
        if (childNode->type == nt_declarator || childNode->type == nt_abstract_declarator)
            _line += " ";
        break;
    case nt_pointer:
        if (childNode->type == nt_pointer)
            _line += " ";
        break;
    case nt_relational_expression:
    case nt_shift_expression:
        if (node->u.binary_expression.right == childNode)
            _line += " " + tokenToString(node->u.binary_expression.op) + " ";
        break;
    case nt_selection_statement_if:
        if (childNode == node->u.selection_statement.statement)
            _line += ")";
        if (childNode == node->u.selection_statement.statement_else)
            _line += "else";
        if (childNode->type != nt_assignment_expression) {
            if (childNode->type == nt_compound_statement)
                _line += " ";
            else
                newlineIncIndent();
        }
        break;
    case nt_struct_declaration:
        if (childNode->type == nt_struct_declarator)
            _line += " ";
        break;
    case nt_struct_declarator:
        if (childNode->type == nt_constant_expression)
            _line += ": ";
        break;
    case nt_type_name:
        if (childNode->type == nt_abstract_declarator)
            _line += " ";
        break;
    case nt_unary_expression:
        if (childNode->type == nt_cast_expression)
            _line += tokenToString(node->u.unary_expression.unary_operator);
        break;
    default:
        break;
    }
}


void ASTSourcePrinter::afterChild(const ASTNode *node, const ASTNode *childNode)
{
    switch (node->type) {
    case nt_builtin_function_expect:
    case nt_builtin_function_object_size:
        if (childNode->type == nt_assignment_expression)
            _line += ", ";
        break;
    case nt_cast_expression:
        if (childNode->type == nt_type_name)
            _line += ") ";
        break;
    case nt_conditional_expression:
        if (childNode->type == nt_logical_or_expression &&
            node->u.conditional_expression.conditional_expression)
            _line += " ? ";
        break;
    case nt_declarator:
        if (childNode->type == nt_pointer)
            _line += " ";
        break;
    case nt_direct_abstract_declarator:
        if (childNode->type == nt_abstract_declarator)
            _line += ")";
        break;
    case nt_enum_specifier:
        if (childNode->type == nt_enumerator) {
            newlineDecIndent();
            _line += "}";
        }
        break;
    case nt_initializer:
        if (childNode->type == nt_initializer) {
            newlineDecIndent();
            _line += "}";
        }
        break;
    case nt_iteration_statement_do:
        if (node->u.iteration_statement.statement != childNode) {
            _line += ");";
            newlineIndent();
        }
        break;
    case nt_iteration_statement_for:
    case nt_iteration_statement_while:
        if (node->u.iteration_statement.statement == childNode) {
            if (childNode->type != nt_compound_statement)
                newlineDecIndent();
            else
                newlineIndent();
        }
        break;
    case nt_labeled_statement_case:
        if (childNode->type == nt_constant_expression) {
            if (childNode == node->u.labeled_statement.constant_expression &&
                node->u.labeled_statement.constant_expression2)
                _line += " ... ";
            else  {
                _line += ":";
                newlineIncIndent();
            }
        }
        break;
    case nt_lvalue:
        if (childNode->type == nt_type_name)
            _line += ") ";
        break;
    case nt_selection_statement_if:
        if (childNode->type != nt_assignment_expression &&
            childNode->type != nt_compound_statement)
            newlineDecIndent();
        break;
    case nt_selection_statement_switch:
        if (childNode->type == nt_assignment_expression)
            _line += ") ";
        break;
    case nt_struct_or_union_specifier:
        if (childNode == node->u.struct_or_union_specifier.struct_or_union) {
            if (node->u.struct_or_union_specifier.identifier)
                _line += " " + tokenToString(node->u.struct_or_union_specifier.identifier);
            if (node->u.struct_or_union_specifier.isDefinition) {
                _line += " {";
                newlineIncIndent();
            }
        }
        break;
    default:
        break;

    }
}


void ASTSourcePrinter::afterChildren(const ASTNode *node, int flags)
{
    Q_UNUSED(flags);
    QString nodeId = QString::number((quint64)node, 16);

    // Do we have a terminal token?
    switch (node->type) {
    case nt_abstract_declarator_suffix_brackets:
        _line += "]";
        break;
    case nt_abstract_declarator_suffix_parens:
        _line += ")";
        break;
    case nt_assembler_parameter:
        _line += ")";
        break;
    case nt_assembler_statement:
        _line += ");";
        newlineIndent();
        break;
    case nt_builtin_function_alignof:
        if (node->u.builtin_function_sizeof.type_name)
            _line += ")";
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
        _line += ")";
        break;
    case nt_builtin_function_sizeof:
        if (node->u.builtin_function_alignof.type_name)
            _line += ")";
        break;
    case nt_compound_braces_statement:
        newlineDecIndent();
        _line += "})";
        break;
    case nt_compound_statement:
        newlineDecIndent();
        _line += "}";
        newlineIndent();
        break;
    case nt_declaration:
        _line += ";";
        newlineIndent();
        break;
    case nt_declarator:
        if (node->u.declarator.isFuncDeclarator)
            _line += ")";
        break;
    case nt_declarator_suffix_brackets:
        _line += "]";
        break;
    case nt_declarator_suffix_parens:
        _line += ")";
        break;
    case nt_designated_initializer:
        if (!node->u.designated_initializer.identifier)
            _line += "]";
        break;
    case nt_expression_statement:
        _line += ";";
        if (node->parent && node->parent->type == nt_iteration_statement_for)
            _line += " ";
        else
            newlineIndent();
        break;
    case nt_jump_statement_goto:
    case nt_jump_statement_return:
        _line += ";";
        newlineIndent();
        break;
    case nt_labeled_statement_case:
    case nt_labeled_statement_default:
        newlineDecIndent();
        break;
    case nt_parameter_type_list:
        if (node->u.parameter_type_list.openArgs)
            _line += ", ...";
        break;
    case nt_postfix_expression_brackets:
        _line += "]";
        break;
    case nt_postfix_expression_parens:
        _line += ")";
        break;
    case nt_primary_expression:
        if (node->u.primary_expression.expression)
            _line += ")";
        break;
    case nt_struct_declaration:
        _line += ";";
        newlineIndent();
        break;
    case nt_struct_or_union_specifier:
        if (node->u.struct_or_union_specifier.isDefinition) {
            newlineDecIndent();
            _line += "}";
        }
        break;
    case nt_typeof_specification:
        _line += ")";
        break;
    default:
        break;
   }
}


QString ASTSourcePrinter::toString(bool lineNo)
{
    _line.clear();
    _out.clear();
    _lineIndent = _indent = 0;
    _currNode = _ast && _ast->rootNodes() ? _ast->rootNodes()->item : 0;
    _prefixLineNo = lineNo;

    if (_ast) {
        walkTree();
        if (!_line.isEmpty())
            newlineIndent();
    }

    return _out;
}


QString ASTSourcePrinter::toString(const ASTNode* node, bool lineNo)
{
    _line.clear();
    _out.clear();
    _lineIndent = _indent = 0;
    _currNode = node;
    _prefixLineNo = lineNo;

    if (_currNode) {
        walkTree(node);
        if (!_line.isEmpty())
            newlineIndent();
    }
    return _out;
}
