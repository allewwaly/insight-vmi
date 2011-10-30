/*
 * astnode.h
 *
 *  Created on: 15.04.2011
 *      Author: chrschn
 */

#ifndef ASTNODE_H_
#define ASTNODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <antlr3.h>
#include <astsymboltypes.h>

#ifdef __cplusplus
class ASTScope;
typedef ASTScope* pASTScope;
#else
struct ASTScope;
typedef struct ASTScope* pASTScope;
#endif

// Keep in sync with const char* node_type_names[] !!!
enum ASTNodeType
{
    nt_undefined = 0,
    nt_abstract_declarator_suffix_brackets,
    nt_abstract_declarator_suffix_parens,
    nt_abstract_declarator,
    nt_additive_expression,
    nt_and_expression,
    nt_assembler_parameter,
    nt_assembler_statement,
    nt_assignment_expression,
    nt_builtin_function_alignof,
    nt_builtin_function_choose_expr,
    nt_builtin_function_constant_p,
    nt_builtin_function_expect,
    nt_builtin_function_extract_return_addr,
    nt_builtin_function_object_size,
    nt_builtin_function_offsetof,
    nt_builtin_function_prefetch,
    nt_builtin_function_return_address,
    nt_builtin_function_sizeof,
    nt_builtin_function_types_compatible_p,
    nt_builtin_function_va_arg,
    nt_builtin_function_va_copy,
    nt_builtin_function_va_end,
    nt_builtin_function_va_start,
    nt_cast_expression,
    nt_compound_braces_statement,
    nt_compound_statement,
    nt_conditional_expression,
    nt_constant_char,
    nt_constant_expression,
    nt_constant_float,
    nt_constant_int,
    nt_constant_string,
    nt_declaration_specifier,
    nt_declaration,
    nt_declarator_suffix_brackets,
    nt_declarator_suffix_parens,
    nt_declarator,
    nt_designated_initializer,
    nt_direct_abstract_declarator,
    nt_direct_declarator,
    nt_enum_specifier,
    nt_enumerator,
    nt_equality_expression,
    nt_exclusive_or_expression,
    nt_expression_statement,
    nt_external_declaration,
    nt_function_definition,
    nt_inclusive_or_expression,
    nt_init_declarator,
    nt_initializer,
    nt_iteration_statement_do,
    nt_iteration_statement_for,
    nt_iteration_statement_while,
    nt_jump_statement_break,
    nt_jump_statement_continue,
    nt_jump_statement_goto,
    nt_jump_statement_return,
    nt_labeled_statement_case,
    nt_labeled_statement_default,
    nt_labeled_statement,
    nt_logical_and_expression,
    nt_logical_or_expression,
    nt_lvalue,
    nt_multiplicative_expression,
    nt_parameter_declaration,
    nt_parameter_type_list,
    nt_pointer,
    nt_postfix_expression_arrow,
    nt_postfix_expression_brackets,
    nt_postfix_expression_dec,
    nt_postfix_expression_dot,
    nt_postfix_expression_inc,
    nt_postfix_expression_parens,
    nt_postfix_expression,
    nt_primary_expression,
    nt_relational_expression,
    nt_selection_statement_if,
    nt_selection_statement_switch,
    nt_shift_expression,
    nt_storage_class_specifier,
    nt_struct_declaration,
    nt_struct_declarator,
    nt_struct_or_union_specifier,
    nt_struct_or_union_struct,
    nt_struct_or_union_union,
    nt_type_id,
    nt_type_name,
    nt_type_qualifier,
    nt_type_specifier,
    nt_typeof_specification,
    nt_unary_expression_builtin,
    nt_unary_expression_dec,
    nt_unary_expression_inc,
    nt_unary_expression_op,
    nt_unary_expression,
    nt_size
};


struct ASTNode;
struct ASTNodeList
{
    struct ASTNode* item;
    struct ASTNodeList* next;
};

typedef struct ASTNodeList* pASTNodeList;


struct ASTTokenList
{
    pANTLR3_COMMON_TOKEN item;
    struct ASTTokenList* next;
};

typedef struct ASTTokenList* pASTTokenList;


struct ASTNode
{
	enum ASTNodeType type;
    struct ASTNode* parent;
    pASTScope scope;
    pASTScope childrenScope;
    pANTLR3_COMMON_TOKEN start;
    pANTLR3_COMMON_TOKEN stop;

    union Children
    {
        struct
        {
            struct ASTNode* function_definition;
            struct ASTNode* declaration;
            struct ASTNode* assembler_statement;
        } external_declaration;

        struct
        {
            struct ASTNode* declaration_specifier;
            struct ASTNode* declarator;
            struct ASTNode* compound_statement;
        } function_definition;

        struct
        {
            ANTLR3_BOOLEAN isTypedef;
            struct ASTNode* declaration_specifier;
            struct ASTNodeList* init_declarator_list;
        } declaration;

        struct
        {
            struct ASTNodeList* declaration_specifier_list;
        } declaration_specifier;

        struct
        {
            struct ASTNode* declarator;
            struct ASTNode* initializer;
        } init_declarator;

        struct
        {
            pANTLR3_COMMON_TOKEN token;
        } storage_class_specifier;

        struct
        {
            struct ASTTokenList* builtin_type_list;
            struct ASTNode* typeof_specification;
            struct ASTNode* struct_or_union_specifier;
            struct ASTNode* enum_specifier;
            struct ASTNode* type_id;
        } type_specifier;

        struct
        {
            struct ASTNode* assignment_expression;
            struct ASTNode* parameter_declaration;
        } typeof_specification;

        struct
        {
            pANTLR3_COMMON_TOKEN identifier;
        } type_id;

        struct
        {
            struct ASTNode* struct_or_union;
            pANTLR3_COMMON_TOKEN identifier;
            struct ASTNodeList* struct_declaration_list;
            ANTLR3_BOOLEAN isDefinition;
        } struct_or_union_specifier;

        struct
        {
            struct ASTNodeList* specifier_qualifier_list;
            struct ASTNodeList* struct_declarator_list;
        } struct_declaration;

        struct
        {
            struct ASTNode* declarator;
            struct ASTNode* constant_expression;
        } struct_declarator;

        struct
        {
            pANTLR3_COMMON_TOKEN identifier;
            struct ASTNodeList* enumerator_list;
        } enum_specifier;

        struct
        {
            pANTLR3_COMMON_TOKEN identifier;
            struct ASTNode* constant_expression;
        } enumerator;

        struct
        {
            pANTLR3_COMMON_TOKEN token;
        } type_qualifier;

        struct
        {
            struct ASTNode* pointer;
            struct ASTNode* direct_declarator;
            ANTLR3_BOOLEAN isFuncDeclarator;
        } declarator;

        struct
        {
            pANTLR3_COMMON_TOKEN identifier;
            struct ASTNode* declarator;
            struct ASTNodeList* declarator_suffix_list;
        } direct_declarator;

        struct
        {
            struct ASTNode* constant_expression;
            struct ASTNode* parameter_type_list;
            struct ASTTokenList* identifier_list;
        } declarator_suffix;

        struct
        {
            struct ASTTokenList* type_qualifier_list;
            struct ASTNode* pointer;
        } pointer;

        struct
        {
            struct ASTNodeList* parameter_list;
            ANTLR3_BOOLEAN openArgs;
        } parameter_type_list;

        struct
        {
            struct ASTNode* declaration_specifier;
            struct ASTNodeList* declarator_list;
        } parameter_declaration;

        struct
        {
            struct ASTNodeList* specifier_qualifier_list;
            struct ASTNode* abstract_declarator;
        } type_name;

        struct
        {
            struct ASTNode* pointer;
            struct ASTNode* direct_abstract_declarator;
        } abstract_declarator;

        struct
        {
            struct ASTNode* abstract_declarator;
            struct ASTNodeList* abstract_declarator_suffix_list;
        } direct_abstract_declarator;

        struct
        {
            struct ASTNode* constant_expression;
            struct ASTNode* parameter_type_list;
        } abstract_declarator_suffix;

        struct
        {
            struct ASTNode* assignment_expression;
            struct ASTNodeList* initializer_list;
        } initializer;

        struct
        {
            struct ASTNode* cast_expression;
            struct ASTNode* initializer;
            struct ASTNode* type_name;
            struct ASTNode* unary_expression;
        } cast_expression;

        struct
        {
            struct ASTNode* postfix_expression;
            struct ASTNode* unary_expression;
            pANTLR3_COMMON_TOKEN unary_operator;
            struct ASTNode* cast_expression;
            struct ASTNode* builtin_function;
        } unary_expression;

        struct
        {
            struct ASTNode* unary_expression;
            struct ASTNode* type_name;
        } builtin_function_alignof;

        struct
        {
            struct ASTNode* constant_expression;
            struct ASTNode* assignment_expression1;
            struct ASTNode* assignment_expression2;
        } builtin_function_choose_expr;

        struct
        {
            struct ASTNode* unary_expression;
        } builtin_function_constant_p;

        struct
        {
            struct ASTNode* assignment_expression;
            struct ASTNode* constant;
        } builtin_function_expect;

        struct
        {
            struct ASTNode* assignment_expression;
        } builtin_function_extract_return_addr;

        struct
        {
            struct ASTNode* assignment_expression;
            struct ASTNode* constant;
        } builtin_function_object_size;

        struct
        {
            struct ASTNode* type_name;
            struct ASTNode* postfix_expression;
        } builtin_function_offsetof;

        struct
        {
            struct ASTNode* assignment_expression;
            struct ASTNode* constant_expression1;
            struct ASTNode* constant_expression2;
        } builtin_function_prefetch;

        struct
        {
            struct ASTNode* constant;
        } builtin_function_return_address;

        struct
        {
            struct ASTNode* type_name1;
            struct ASTNode* type_name2;
        } builtin_function_types_compatible_p;

        struct
        {
            struct ASTNode* assignment_expression;
            struct ASTNode* assignment_expression2;
            struct ASTNode* type_name;
        } builtin_function_va_xxx;

        struct
        {
            struct ASTNode* unary_expression;
            struct ASTNode* type_name;
        } builtin_function_sizeof;

        struct
        {
            struct ASTNode* primary_expression;
            struct ASTNodeList* postfix_expression_suffix_list;
        } postfix_expression;

        struct
        {
            struct ASTNodeList* expression;
            struct ASTNodeList* argument_expression_list;
            pANTLR3_COMMON_TOKEN identifier;
        } postfix_expression_suffix;

        struct
        {
            ANTLR3_BOOLEAN hasDot;
            pANTLR3_COMMON_TOKEN identifier;
            struct ASTNode* constant;
            struct ASTNodeList* expression;
            struct ASTNode* compound_braces_statement;
        } primary_expression;

        struct
        {
            pANTLR3_COMMON_TOKEN literal;
            struct ASTTokenList* string_token_list;
        } constant;

        struct
        {
            struct ASTNode* conditional_expression;
        } constant_expression;

        struct
        {
            struct ASTNode* lvalue;
            struct ASTNodeList* designated_initializer_list;
            pANTLR3_COMMON_TOKEN assignment_operator;
            struct ASTNode* assignment_expression;
            struct ASTNode* initializer;
            struct ASTNode* conditional_expression;
        } assignment_expression;

        struct
        {
            struct ASTNode* constant_expression1;
            struct ASTNode* constant_expression2;
        } designated_initializer;

        struct
        {
            struct ASTNode* type_name;
            struct ASTNode* lvalue;
            struct ASTNode* unary_expression;
        } lvalue;

        struct
        {
            struct ASTNode* logical_or_expression;
            struct ASTNodeList* expression;
            struct ASTNode* conditional_expression;
        } conditional_expression;

        struct
        {
            pANTLR3_COMMON_TOKEN op;
            struct ASTNode* left;
            struct ASTNode* right;
        } binary_expression;

        struct
        {
            pANTLR3_COMMON_TOKEN identifier;
            struct ASTNode* constant_expression;
            struct ASTNode* constant_expression2;
            struct ASTNode* statement;
        } labeled_statement;

        struct
        {
            struct ASTNodeList* declaration_or_statement_list;
        } compound_statement;

        struct
        {
            struct ASTNodeList* declaration_or_statement_list;
        } compound_braces_statement;

        struct
        {
            struct ASTNodeList* expression;
        } expression_statement;

        struct
        {
            struct ASTNodeList* expression;
            struct ASTNode* statement;
            struct ASTNode* statement_else;
        } selection_statement;

        struct
        {
            pASTNodeList assignment_expression;
        } expression;

        struct
        {
            struct ASTNode* expression_statement_init;
            struct ASTNode* expression_statement_cond;
            struct ASTNodeList* expression;
            struct ASTNode* statement;
        } iteration_statement;

        struct
        {
            struct ASTNode* unary_expression;
            struct ASTNode* initializer;
        } jump_statement;

        struct
        {
            struct ASTTokenList* type_qualifier_list;
            struct ASTTokenList* instructions;
            struct ASTNodeList* input_parameter_list;
            struct ASTNodeList* output_parameter_list;
        } assembler_statement;

        struct
        {
            pANTLR3_COMMON_TOKEN alias;
            struct ASTTokenList* constraint_list;
            struct ASTNode* assignment_expression;
        } assembler_parameter;
    } u;

};

typedef struct ASTNode* pASTNode;

const char* ast_node_type_to_str(const struct ASTNode* node);
const char* ast_node_type_to_str2(int type);

#ifdef __cplusplus
}
#endif


#endif /* ASTNODE_H_ */
