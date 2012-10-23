/*
 * astnode.cpp
 *
 *  Created on: 15.04.2011
 *      Author: chrschn
 */

#include <astnode.h>

// Keep in sync with enum ASTNodeType!!!
static const char* node_type_names[] =
{
    "undefined",
    "abstract_declarator_suffix_brackets",
    "abstract_declarator_suffix_parens",
    "abstract_declarator",
    "additive_expression",
    "and_expression",
    "assembler_parameter",
    "assembler_statement",
    "assignment_expression",
    "builtin_function_alignof",
    "builtin_function_choose_expr",
    "builtin_function_constant_p",
    "builtin_function_expect",
    "builtin_function_extract_return_addr",
    "builtin_function_object_size",
    "builtin_function_offsetof",
    "builtin_function_prefetch",
    "builtin_function_return_address",
    "builtin_function_sizeof",
    "builtin_function_types_compatible_p",
    "builtin_function_va_arg",
    "builtin_function_va_copy",
    "builtin_function_va_end",
    "builtin_function_va_start",
    "cast_expression",
    "compound_braces_statement",
    "compound_statement",
    "conditional_expression",
    "constant_char",
    "constant_expression",
    "constant_float",
    "constant_int",
    "constant_string",
    "declaration_specifier",
    "declaration",
    "declarator_suffix_brackets",
    "declarator_suffix_parens",
    "declarator",
    "designated_initializer",
    "direct_abstract_declarator",
    "direct_declarator",
    "enum_specifier",
    "enumerator",
    "equality_expression",
    "exclusive_or_expression",
    "expression_statement",
    "external_declaration",
    "function_definition",
    "inclusive_or_expression",
    "init_declarator",
    "initializer",
    "iteration_statement_do",
    "iteration_statement_for",
    "iteration_statement_while",
    "jump_statement_break",
    "jump_statement_continue",
    "jump_statement_goto",
    "jump_statement_return",
    "labeled_statement_case",
    "labeled_statement_default",
    "labeled_statement",
    "logical_and_expression",
    "logical_or_expression",
    "lvalue",
    "multiplicative_expression",
    "parameter_declaration",
    "parameter_type_list",
    "pointer",
    "postfix_expression_arrow",
    "postfix_expression_brackets",
    "postfix_expression_dec",
    "postfix_expression_dot",
    "postfix_expression_inc",
    "postfix_expression_parens",
    "postfix_expression",
    "primary_expression",
    "relational_expression",
    "selection_statement_if",
    "selection_statement_switch",
    "shift_expression",
    "storage_class_specifier",
    "struct_declaration",
    "struct_declarator",
    "struct_or_union_specifier",
    "struct_or_union_struct",
    "struct_or_union_union",
    "type_id",
    "type_name",
    "type_qualifier",
    "type_specifier",
    "typeof_specification",
    "unary_expression_builtin",
    "unary_expression_dec",
    "unary_expression_inc",
    "unary_expression_op",
    "unary_expression",
};


const char* ast_node_type_to_str(const struct ASTNode* node)
{
    if (!node)
        return 0;
    return ast_node_type_to_str2(node->type);
}


const char* ast_node_type_to_str2(int type)
{
    if (type < 0 || type >= nt_size)
        return 0;
    return node_type_names[type];
}
