/*
 * astwalker.cpp
 *
 *  Created on: 28.03.2011
 *      Author: chrschn
 */

#include <astwalker.h>
#include <abstractsyntaxtree.h>
#include <debug.h>


ASTWalker::ASTWalker(AbstractSyntaxTree* ast) :
    _ast(ast), _stopWalking(false)
{
}


ASTWalker::~ASTWalker()
{
}


int ASTWalker::walkTree()
{
    _stopWalking = false;
    return _ast ? walkTree(_ast->rootNodes()) : 0;
}


int ASTWalker::walkTree(const ASTNodeList *head)
{
	int visits = 0;
	for (const ASTNodeList *p = head; p && !_stopWalking; p = p->next) {
        // List flags of current item
        int flags = wfIsList;
        if (p == head)
            flags |= wfFirstInList;
        if (!p->next)
            flags |= wfLastInList;
        // Visit the list element
        visits += walkTree(p->item, flags);
    }
    return visits;
}


int ASTWalker::walkTree(const ASTNode *node, int flags)
{
    if (!node || _stopWalking)
        return 0;

//	debugmsg(QString("Visiting %1 (%2:%3)")
//				.arg(ast_node_type_to_str(node))
//				.arg(node->start->line)
//				.arg(node->start->charPosition));

    // Invoke the virtual handler functions
    if (node->parent && (!(flags & wfIsList) || (flags & wfFirstInList)))
        beforeChild(node->parent, node);
    beforeChildren(node, flags);

    int visits = 1;

    // Recurse to all children
    switch (node->type) {
    case nt_abstract_declarator: {
        visits += walkTree(node->u.abstract_declarator.pointer);
        visits += walkTree(node->u.abstract_declarator.direct_abstract_declarator);
        break;
    }
    case nt_abstract_declarator_suffix_brackets:
    case nt_abstract_declarator_suffix_parens: {
        visits += walkTree(node->u.abstract_declarator_suffix.constant_expression);
        visits += walkTree(node->u.abstract_declarator_suffix.parameter_type_list);
        break;
    }
    case nt_assembler_parameter: {
        visits += walkTree(node->u.assembler_parameter.assignment_expression);
        break;
    }
    case nt_assembler_statement: {
        visits += walkTree(node->u.assembler_statement.input_parameter_list);
        visits += walkTree(node->u.assembler_statement.output_parameter_list);
        break;
    }
    case nt_assignment_expression: {
        visits += walkTree(node->u.assignment_expression.lvalue);
        visits += walkTree(node->u.assignment_expression.assignment_expression);
        visits += walkTree(node->u.assignment_expression.designated_initializer_list);
        visits += walkTree(node->u.assignment_expression.initializer);
        visits += walkTree(node->u.assignment_expression.conditional_expression);
        break;
    }
    case nt_builtin_function_alignof: {
        visits += walkTree(node->u.builtin_function_alignof.unary_expression);
        visits += walkTree(node->u.builtin_function_alignof.type_name);
        break;
    }
    case nt_builtin_function_choose_expr: {
        visits += walkTree(node->u.builtin_function_choose_expr.constant_expression);
        visits += walkTree(node->u.builtin_function_choose_expr.assignment_expression1);
        visits += walkTree(node->u.builtin_function_choose_expr.assignment_expression2);
        break;
    }
    case nt_builtin_function_constant_p: {
        visits += walkTree(node->u.builtin_function_constant_p.unary_expression);
        break;
    }
    case nt_builtin_function_expect: {
        visits += walkTree(node->u.builtin_function_expect.assignment_expression);
        visits += walkTree(node->u.builtin_function_expect.constant);
        break;
    }
    case nt_builtin_function_extract_return_addr: {
        visits += walkTree(node->u.builtin_function_extract_return_addr.assignment_expression);
        break;
    }
    case nt_builtin_function_object_size: {
        visits += walkTree(node->u.builtin_function_object_size.assignment_expression);
        visits += walkTree(node->u.builtin_function_object_size.constant);
        break;
    }
    case nt_builtin_function_offsetof: {
        visits += walkTree(node->u.builtin_function_offsetof.type_name);
        visits += walkTree(node->u.builtin_function_offsetof.postfix_expression);
        break;
    }
    case nt_builtin_function_prefetch: {
        visits += walkTree(node->u.builtin_function_prefetch.assignment_expression);
        visits += walkTree(node->u.builtin_function_prefetch.constant_expression1);
        visits += walkTree(node->u.builtin_function_prefetch.constant_expression2);
        break;
    }
    case nt_builtin_function_return_address: {
        visits += walkTree(node->u.builtin_function_return_address.constant);
        break;
    }
    case nt_builtin_function_sizeof: {
        visits += walkTree(node->u.builtin_function_sizeof.type_name);
        visits += walkTree(node->u.builtin_function_sizeof.unary_expression);
        break;
    }
    case nt_builtin_function_types_compatible_p: {
        visits += walkTree(node->u.builtin_function_types_compatible_p.type_name1);
        visits += walkTree(node->u.builtin_function_types_compatible_p.type_name2);
        break;
    }
    case nt_builtin_function_va_arg:
    case nt_builtin_function_va_copy:
    case nt_builtin_function_va_end:
    case nt_builtin_function_va_start: {
        visits += walkTree(node->u.builtin_function_va_xxx.assignment_expression);
        visits += walkTree(node->u.builtin_function_va_xxx.assignment_expression2);
        visits += walkTree(node->u.builtin_function_va_xxx.type_name);
        break;
    }
    case nt_cast_expression: {
        visits += walkTree(node->u.cast_expression.type_name);
        visits += walkTree(node->u.cast_expression.cast_expression);
        visits += walkTree(node->u.cast_expression.initializer);
        visits += walkTree(node->u.cast_expression.unary_expression);
        break;
    }
    case nt_compound_statement: {
        visits += walkTree(node->u.compound_statement.declaration_or_statement_list);
        break;
    }
    case nt_compound_braces_statement: {
        visits += walkTree(node->u.compound_braces_statement.declaration_or_statement_list);
        break;
    }
    case nt_conditional_expression: {
        visits += walkTree(node->u.conditional_expression.logical_or_expression);
        visits += walkTree(node->u.conditional_expression.expression);
        visits += walkTree(node->u.conditional_expression.conditional_expression);
        break;
    }
    case nt_constant_expression: {
        visits += walkTree(node->u.constant_expression.conditional_expression);
        break;
    }
    case nt_constant_int:
    case nt_constant_char:
    case nt_constant_string:
    case nt_constant_float: {
        // No child nodes
        break;
    }
    case nt_declaration: {
        visits += walkTree(node->u.declaration.declaration_specifier);
        visits += walkTree(node->u.declaration.init_declarator_list);
        break;
    }
    case nt_declaration_specifier: {
        visits += walkTree(node->u.declaration_specifier.declaration_specifier_list);
        break;
    }
    case nt_declarator: {
        visits += walkTree(node->u.declarator.pointer);
        visits += walkTree(node->u.declarator.direct_declarator);
        break;
    }
    case nt_declarator_suffix_brackets:
    case nt_declarator_suffix_parens: {
        visits += walkTree(node->u.declarator_suffix.constant_expression);
        visits += walkTree(node->u.declarator_suffix.parameter_type_list);
        break;
    }
    case nt_designated_initializer: {
    	visits += walkTree(node->u.designated_initializer.constant_expression1);
    	visits += walkTree(node->u.designated_initializer.constant_expression2);
    	break;
    }
    case nt_direct_abstract_declarator: {
        visits += walkTree(node->u.direct_abstract_declarator.abstract_declarator);
        visits += walkTree(node->u.direct_abstract_declarator.abstract_declarator_suffix_list);
        break;
    }
    case nt_direct_declarator: {
        visits += walkTree(node->u.direct_declarator.declarator);
        visits += walkTree(node->u.direct_declarator.declarator_suffix_list);
        break;
    }
    case nt_enum_specifier: {
        visits += walkTree(node->u.enum_specifier.enumerator_list);
        break;
    }
    case nt_enumerator: {
        visits += walkTree(node->u.enumerator.constant_expression);
        break;
    }
    case nt_expression_statement: {
        visits += walkTree(node->u.expression_statement.expression);
        break;
    }
    case nt_external_declaration: {
        visits += walkTree(node->u.external_declaration.function_definition);
        visits += walkTree(node->u.external_declaration.declaration);
        visits += walkTree(node->u.external_declaration.assembler_statement);
        break;
    }
    case nt_function_definition: {
        visits += walkTree(node->u.function_definition.declaration_specifier);
        visits += walkTree(node->u.function_definition.declarator);
        visits += walkTree(node->u.function_definition.compound_statement);
        break;
    }
    case nt_init_declarator: {
        visits += walkTree(node->u.init_declarator.declarator);
        visits += walkTree(node->u.init_declarator.initializer);
        break;
    }
    case nt_initializer: {
        visits += walkTree(node->u.initializer.assignment_expression);
        visits += walkTree(node->u.initializer.initializer_list);
        break;
    }
    case nt_iteration_statement_while:
    case nt_iteration_statement_for: {
        visits += walkTree(node->u.iteration_statement.expression_statement_init);
        visits += walkTree(node->u.iteration_statement.expression_statement_cond);
        visits += walkTree(node->u.iteration_statement.expression);
        visits += walkTree(node->u.iteration_statement.statement);
        break;
    }
    case nt_iteration_statement_do: {
        visits += walkTree(node->u.iteration_statement.statement);
        visits += walkTree(node->u.iteration_statement.expression);
        break;
    }
    case nt_jump_statement_goto:
    case nt_jump_statement_continue:
    case nt_jump_statement_break:
    case nt_jump_statement_return: {
        visits += walkTree(node->u.jump_statement.unary_expression);
        visits += walkTree(node->u.jump_statement.initializer);
        break;
    }
    case nt_labeled_statement:
    case nt_labeled_statement_case:
    case nt_labeled_statement_default: {
        visits += walkTree(node->u.labeled_statement.constant_expression);
        visits += walkTree(node->u.labeled_statement.constant_expression2);
        visits += walkTree(node->u.labeled_statement.statement);
        break;
    }
    case nt_lvalue: {
        visits += walkTree(node->u.lvalue.type_name);
        visits += walkTree(node->u.lvalue.lvalue);
        visits += walkTree(node->u.lvalue.unary_expression);
        break;
    }
    case nt_parameter_type_list: {
        visits += walkTree(node->u.parameter_type_list.parameter_list);
        break;
    }
    case nt_parameter_declaration: {
        visits += walkTree(node->u.parameter_declaration.declaration_specifier);
        visits += walkTree(node->u.parameter_declaration.declarator_list);
        break;
    }
    case nt_pointer: {
        visits += walkTree(node->u.pointer.pointer);
        break;
    }
    case nt_postfix_expression: {
        visits += walkTree(node->u.postfix_expression.primary_expression);
        visits += walkTree(node->u.postfix_expression.postfix_expression_suffix_list);
        break;
    }
    case nt_postfix_expression_brackets:
    case nt_postfix_expression_parens:
    case nt_postfix_expression_dot:
//    case nt_postfix_expression_star:
    case nt_postfix_expression_arrow:
    case nt_postfix_expression_inc:
    case nt_postfix_expression_dec: {
        visits += walkTree(node->u.postfix_expression_suffix.expression);
        visits += walkTree(node->u.postfix_expression_suffix.argument_expression_list);
        break;
    }
    case nt_primary_expression: {
        visits += walkTree(node->u.primary_expression.constant);
        visits += walkTree(node->u.primary_expression.expression);
        visits += walkTree(node->u.primary_expression.compound_braces_statement);
        break;
    }
    case nt_storage_class_specifier: {
        // No child nodes
        break;
    }
    case nt_struct_declaration: {
        visits += walkTree(node->u.struct_declaration.specifier_qualifier_list);
        visits += walkTree(node->u.struct_declaration.struct_declarator_list);
        break;
    }
    case nt_struct_declarator: {
        visits += walkTree(node->u.struct_declarator.declarator);
        visits += walkTree(node->u.struct_declarator.constant_expression);
        break;
    }
    case nt_struct_or_union_specifier: {
        visits += walkTree(node->u.struct_or_union_specifier.struct_or_union);
        visits += walkTree(node->u.struct_or_union_specifier.struct_declaration_list);
        break;
    }
    case nt_struct_or_union_struct:
    case nt_struct_or_union_union: {
        // No child nodes
        break;
    }
    case nt_type_id: {
        // No child nodes
        break;
    }
    case nt_type_qualifier: {
        // No child nodes
        break;
    }
    case nt_selection_statement_if:
    case nt_selection_statement_switch: {
        visits += walkTree(node->u.selection_statement.expression);
        visits += walkTree(node->u.selection_statement.statement);
        visits += walkTree(node->u.selection_statement.statement_else);
        break;
    }
    case nt_type_specifier: {
        visits += walkTree(node->u.type_specifier.typeof_specification);
        visits += walkTree(node->u.type_specifier.struct_or_union_specifier);
        visits += walkTree(node->u.type_specifier.enum_specifier);
        visits += walkTree(node->u.type_specifier.type_id);
        break;
    }
    case nt_typeof_specification: {
        visits += walkTree(node->u.typeof_specification.assignment_expression);
        visits += walkTree(node->u.typeof_specification.parameter_declaration);
        break;
    }
    case nt_type_name: {
        visits += walkTree(node->u.type_name.specifier_qualifier_list);
        visits += walkTree(node->u.type_name.abstract_declarator);
        break;
    }
    case nt_unary_expression:
    case nt_unary_expression_dec:
    case nt_unary_expression_inc:
    case nt_unary_expression_op:
    case nt_unary_expression_builtin: {
        visits += walkTree(node->u.unary_expression.unary_expression);
        visits += walkTree(node->u.unary_expression.cast_expression);
        visits += walkTree(node->u.unary_expression.postfix_expression);
        visits += walkTree(node->u.unary_expression.builtin_function);
        break;
    }
    // all binary expressions
    case nt_additive_expression:
    case nt_and_expression:
    case nt_equality_expression:
    case nt_exclusive_or_expression:
    case nt_inclusive_or_expression:
    case nt_logical_or_expression:
    case nt_logical_and_expression:
    case nt_multiplicative_expression:
    case nt_relational_expression:
    case nt_shift_expression: {
        visits += walkTree(node->u.binary_expression.left);
        visits += walkTree(node->u.binary_expression.right);
        break;
    }
    default: {
        debugerr("Unhandled node type: " << ast_node_type_to_str(node) << " (" << node->type << ")");
        break;
    }
    }

    // Invoke the virtual handler functions
    afterChildren(node, flags);
    if (node->parent && (!(flags & wfIsList) || (flags & wfLastInList)))
        afterChild(node->parent, node);

    return visits;
}


