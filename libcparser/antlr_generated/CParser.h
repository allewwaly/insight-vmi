/** \file
 *  This C header file was generated by $ANTLR version 3.0.1
 *
 *     -  From the grammar source file : src/C.g
 *     -                            On : 2012-03-21 01:26:18
 *     -                for the parser : CParserParser *
 * Editing it, at least manually, is not wise. 
 *
 * C language generator and runtime by Jim Idle, jimi|hereisanat|idle|dotgoeshere|ws.
 *
 * View this file with tabs set to 8 (:set ts=8 in gvim) and indent at 4 (:set sw=4 in gvim)
 *
 * The parser CParserhas the callable functions (rules) shown below,
 * which will invoke the code for the associated rule in the source grammar
 * assuming that the input stream is pointing to a token/text stream that could begin
 * this rule.
 * 
 * For instance if you call the first (topmost) rule in a parser grammar, you will
 * get the results of a full parse, but calling a rule half way through the grammar will
 * allow you to pass part of a full token stream to the parser, such as for syntax checking
 * in editors and so on.
 *
 * The parser entry points are called indirectly (by function pointer to function) via
 * a parser context typedef pCParser, which is returned from a call to CParserNew().
 *
 * The entry points for CParser are  as follows:
 *
 *  - pASTNodeList      pCParser->translation_unit(pCParser)
 *  - pASTNode      pCParser->external_declaration(pCParser)
 *  - pASTNode      pCParser->function_definition(pCParser)
 *  - pASTNode      pCParser->declaration(pCParser)
 *  - pASTNode      pCParser->declaration_specifier(pCParser)
 *  - pASTNodeList      pCParser->declaration_specifier_list(pCParser)
 *  - pASTNodeList      pCParser->init_declarator_list(pCParser)
 *  - pASTNode      pCParser->init_declarator(pCParser)
 *  - void      pCParser->attribute_specifier(pCParser)
 *  - void      pCParser->attribute(pCParser)
 *  - void      pCParser->attribute_parameter_list(pCParser)
 *  - void      pCParser->attribute_parameter(pCParser)
 *  - void      pCParser->literal_constant_expression(pCParser)
 *  - void      pCParser->literal_conditional_expression(pCParser)
 *  - void      pCParser->literal_logical_or_expression(pCParser)
 *  - void      pCParser->literal_logical_and_expression(pCParser)
 *  - void      pCParser->literal_inclusive_or_expression(pCParser)
 *  - void      pCParser->literal_exclusive_or_expression(pCParser)
 *  - void      pCParser->literal_and_expression(pCParser)
 *  - void      pCParser->literal_equality_expression(pCParser)
 *  - void      pCParser->literal_relational_expression(pCParser)
 *  - void      pCParser->literal_shift_expression(pCParser)
 *  - void      pCParser->literal_additive_expression(pCParser)
 *  - void      pCParser->literal_multiplicative_expression(pCParser)
 *  - void      pCParser->literal_unary_expression(pCParser)
 *  - void      pCParser->literal_builtin_function(pCParser)
 *  - void      pCParser->literal_primary_expression(pCParser)
 *  - pASTNode      pCParser->storage_class_specifier(pCParser)
 *  - pASTNode      pCParser->type_specifier(pCParser)
 *  - pASTTokenList      pCParser->builtin_type_list(pCParser)
 *  - CParser_builtin_type_return      pCParser->builtin_type(pCParser)
 *  - pASTNode      pCParser->typeof_specification(pCParser)
 *  - void      pCParser->typeof_token(pCParser)
 *  - pASTNode      pCParser->type_id(pCParser)
 *  - pASTNode      pCParser->struct_or_union_specifier(pCParser)
 *  - pASTNode      pCParser->struct_or_union(pCParser)
 *  - pASTNodeList      pCParser->struct_declaration_list(pCParser)
 *  - pASTNode      pCParser->struct_declaration(pCParser)
 *  - pASTNodeList      pCParser->specifier_qualifier_list(pCParser)
 *  - pASTNodeList      pCParser->struct_declarator_list(pCParser)
 *  - pASTNode      pCParser->struct_declarator(pCParser)
 *  - pASTNode      pCParser->enum_specifier(pCParser)
 *  - pASTNodeList      pCParser->enumerator_list(pCParser)
 *  - pASTNode      pCParser->enumerator(pCParser)
 *  - CParser_type_qualifier_return      pCParser->type_qualifier(pCParser)
 *  - pASTTokenList      pCParser->type_qualifier_list(pCParser)
 *  - pASTNode      pCParser->declarator(pCParser)
 *  - pANTLR3_COMMON_TOKEN      pCParser->declarator_identifier(pCParser)
 *  - pASTNode      pCParser->function_declarator(pCParser)
 *  - pASTNode      pCParser->direct_declarator(pCParser)
 *  - pASTNodeList      pCParser->declarator_suffix_list(pCParser)
 *  - pASTNode      pCParser->declarator_suffix(pCParser)
 *  - pASTNode      pCParser->pointer(pCParser)
 *  - pASTNode      pCParser->parameter_type_list(pCParser)
 *  - pASTNodeList      pCParser->parameter_list(pCParser)
 *  - pASTNode      pCParser->parameter_declaration(pCParser)
 *  - pASTTokenList      pCParser->identifier_list(pCParser)
 *  - pASTNode      pCParser->type_name(pCParser)
 *  - pASTNode      pCParser->abstract_declarator(pCParser)
 *  - pASTNode      pCParser->direct_abstract_declarator(pCParser)
 *  - pASTNode      pCParser->abstract_declarator_suffix(pCParser)
 *  - pASTNode      pCParser->initializer(pCParser)
 *  - pASTNodeList      pCParser->initializer_list(pCParser)
 *  - pASTNodeList      pCParser->argument_expression_list(pCParser)
 *  - pASTNode      pCParser->additive_expression(pCParser)
 *  - pASTNode      pCParser->multiplicative_expression(pCParser)
 *  - pASTNode      pCParser->cast_expression(pCParser)
 *  - pASTNode      pCParser->unary_expression(pCParser)
 *  - pASTNode      pCParser->builtin_function(pCParser)
 *  - pASTNode      pCParser->builtin_function_alignof(pCParser)
 *  - pASTNode      pCParser->builtin_function_choose_expr(pCParser)
 *  - pASTNode      pCParser->builtin_function_constant_p(pCParser)
 *  - pASTNode      pCParser->builtin_function_expect(pCParser)
 *  - pASTNode      pCParser->builtin_function_extract_return_addr(pCParser)
 *  - pASTNode      pCParser->builtin_function_object_size(pCParser)
 *  - pASTNode      pCParser->builtin_function_offsetof(pCParser)
 *  - pASTNode      pCParser->builtin_function_prefetch(pCParser)
 *  - pASTNode      pCParser->builtin_function_return_address(pCParser)
 *  - pASTNode      pCParser->builtin_function_sizeof(pCParser)
 *  - pASTNode      pCParser->builtin_function_types_compatible_p(pCParser)
 *  - pASTNode      pCParser->builtin_function_va_arg(pCParser)
 *  - pASTNode      pCParser->builtin_function_va_copy(pCParser)
 *  - pASTNode      pCParser->builtin_function_va_end(pCParser)
 *  - pASTNode      pCParser->builtin_function_va_start(pCParser)
 *  - pASTNode      pCParser->postfix_expression(pCParser)
 *  - pASTNodeList      pCParser->postfix_expression_suffix_list(pCParser)
 *  - pASTNode      pCParser->postfix_expression_suffix(pCParser)
 *  - CParser_unary_operator_return      pCParser->unary_operator(pCParser)
 *  - pASTNode      pCParser->primary_expression(pCParser)
 *  - pASTNode      pCParser->constant(pCParser)
 *  - pASTNodeList      pCParser->expression(pCParser)
 *  - pASTNode      pCParser->constant_expression(pCParser)
 *  - pASTNode      pCParser->assignment_expression(pCParser)
 *  - pASTNodeList      pCParser->designated_initializer_list(pCParser)
 *  - pASTNode      pCParser->designated_initializer(pCParser)
 *  - pASTNode      pCParser->lvalue(pCParser)
 *  - CParser_assignment_operator_return      pCParser->assignment_operator(pCParser)
 *  - pASTNode      pCParser->conditional_expression(pCParser)
 *  - pASTNode      pCParser->logical_or_expression(pCParser)
 *  - pASTNode      pCParser->logical_and_expression(pCParser)
 *  - pASTNode      pCParser->inclusive_or_expression(pCParser)
 *  - pASTNode      pCParser->exclusive_or_expression(pCParser)
 *  - pASTNode      pCParser->and_expression(pCParser)
 *  - pASTNode      pCParser->equality_expression(pCParser)
 *  - pASTNode      pCParser->relational_expression(pCParser)
 *  - pASTNode      pCParser->shift_expression(pCParser)
 *  - pASTNode      pCParser->statement(pCParser)
 *  - pASTNode      pCParser->labeled_statement(pCParser)
 *  - void      pCParser->local_label_declaration(pCParser)
 *  - pASTNode      pCParser->compound_statement(pCParser)
 *  - pASTNode      pCParser->compound_braces_statement(pCParser)
 *  - pASTNodeList      pCParser->declaration_or_statement_list(pCParser)
 *  - pASTNode      pCParser->expression_statement(pCParser)
 *  - pASTNode      pCParser->selection_statement(pCParser)
 *  - pASTNode      pCParser->iteration_statement(pCParser)
 *  - pASTNode      pCParser->jump_statement(pCParser)
 *  - void      pCParser->assembler_token(pCParser)
 *  - void      pCParser->assembler_register_variable(pCParser)
 *  - pASTNode      pCParser->assembler_statement(pCParser)
 *  - pASTNodeList      pCParser->assembler_parameter_list(pCParser)
 *  - pASTNode      pCParser->assembler_parameter(pCParser)
 *  - pASTTokenList      pCParser->string_token_list(pCParser)
 *  - void      pCParser->string_list(pCParser)
 *  - void      pCParser->string_literals(pCParser)
 *  - void      pCParser->literal(pCParser)
 *  - void      pCParser->integer_literal(pCParser)
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 *
 * The return type for any particular rule is of course determined by the source
 * grammar file.
 */
#ifndef	_CParser_H
#define _CParser_H
/* =============================================================================
 * Standard antlr3 C runtime definitions
 */
#include    <antlr3.h>

/* End of standard antlr 3 runtime definitions
 * =============================================================================
 */

     #include <assert.h>
     #include <ast_interface.h>
     #define BUILDER SCOPE_TOP(translation_unit)->builder
     
     #ifndef __cplusplus
 //        #define DEBUG 1
         #if (DEBUG == 1)
             #if !defined(debugmsg)
                 #define debugmsg(m, ...) printf("(%s:%d) " m, __FILE__, __LINE__, __VA_ARGS__)
                 #define debugerr(m, ...) fprintf(stderr, "(%s:%d) " m, __FILE__, __LINE__, __VA_ARGS__)
                 #if 1
                     #define debuginit()
                     #define debugafter()
                 #else
                     #define debuginit() debugmsg("BACKTRACKING = %d, '%s' at %d:%d, %s\n", \
                         BACKTRACKING, \
                         (char*)LT(1)->getText(LT(1))->chars, \
                         LT(1)->line, \
                         LT(1)->charPosition, \
                         __PRETTY_FUNCTION__)
                     #define debugafter() printf("(%s:%d) Executing AFTER in %s, BACKTRACKING is %d\n", \
                         __FILE__, \
                         __LINE__, \
                         __PRETTY_FUNCTION__, \
                         BACKTRACKING)
                 #endif
             #endif
         #else
             #define debugmsg(...)
             #define debugerr(...)
             #define debuginit()
             #define debugafter()
         #endif
    #endif



#if defined(WIN32) && defined(_MSC_VER)
// Disable: Unreferenced parameter,                - Rules with parameters that are not used
//          constant conditional,                  - ANTLR realizes that a prediction is always true (synpred usually)
//          initialized but unused variable        - tree rewrite vairables declared but not needed
//          Unreferenced local variable            - lexer rulle decalres but does not always use _type
//          potentially unitialized variable used  - retval always returned from a rule 
//
// These are only really displayed at warning level /W4 but that is the code ideal I am aiming at
// and the codegen must generate some of these warnings by necessity, apart from 4100, which is
// usually generated when a parser rule is given a parameter that it does not use. Mostly though
// this is a matter of orthogonality hence I disable that one.
//
#pragma warning( disable : 4100 )
#pragma warning( disable : 4101 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4189 )
#pragma warning( disable : 4701 )
#endif

/* ========================
 * BACKTRACKING IS ENABLED
 * ========================
 */
typedef struct CParser_builtin_type_return_struct
{
    /** Generic return elements for ANTLR3 rules that are not in tree parsers or returning trees
     */
    pANTLR3_COMMON_TOKEN    start;
    pANTLR3_COMMON_TOKEN    stop;   
}
    CParser_builtin_type_return;

typedef struct CParser_type_qualifier_return_struct
{
    /** Generic return elements for ANTLR3 rules that are not in tree parsers or returning trees
     */
    pANTLR3_COMMON_TOKEN    start;
    pANTLR3_COMMON_TOKEN    stop;   
    pASTNode node;
}
    CParser_type_qualifier_return;

typedef struct CParser_unary_operator_return_struct
{
    /** Generic return elements for ANTLR3 rules that are not in tree parsers or returning trees
     */
    pANTLR3_COMMON_TOKEN    start;
    pANTLR3_COMMON_TOKEN    stop;   
}
    CParser_unary_operator_return;

typedef struct CParser_assignment_operator_return_struct
{
    /** Generic return elements for ANTLR3 rules that are not in tree parsers or returning trees
     */
    pANTLR3_COMMON_TOKEN    start;
    pANTLR3_COMMON_TOKEN    stop;   
}
    CParser_assignment_operator_return;



/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the translation_unit scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  CParser_translation_unitPush().
 */
typedef struct  CParser_translation_unit_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct CParser_translation_unit_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    pASTBuilder builder;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    CParser_translation_unit_SCOPE, * pCParser_translation_unit_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the declaration scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  CParser_declarationPush().
 */
typedef struct  CParser_declaration_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct CParser_declaration_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    ANTLR3_BOOLEAN isTypedef;
    pASTNode typedefNode;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    CParser_declaration_SCOPE, * pCParser_declaration_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the init_declarator scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  CParser_init_declaratorPush().
 */
typedef struct  CParser_init_declarator_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct CParser_init_declarator_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    ANTLR3_BOOLEAN isFuncTypedef;
    ANTLR3_BOOLEAN typedefNameFound;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    CParser_init_declarator_SCOPE, * pCParser_init_declarator_SCOPE;
/* ruleAttributeScopeDecl(scope)
 */
/* makeScopeSet() 
 */
 /** Definition of the initializer scope variable tracking
 *  structure. An instance of this structure is created by calling
 *  CParser_initializerPush().
 */
typedef struct  CParser_initializer_SCOPE_struct
{
    /** Function that the user may provide to be called when the
     *  scope is destroyed (so you can free pANTLR3_HASH_TABLES and so on)
     *
     * \param POinter to an instance of this typedef/struct
     */
    void    (ANTLR3_CDECL *free)	(struct CParser_initializer_SCOPE_struct * frame);
    
    /* =============================================================================
     * Programmer defined variables...
     */
    // Required to allow/disallow designated_initializers later on
            ANTLR3_BOOLEAN isInitializer;

    /* End of programmer defined variables
     * =============================================================================
     */
} 
    CParser_initializer_SCOPE, * pCParser_initializer_SCOPE;

/* ruleAttributeScopeFuncMacro(scope)
 */
/** Macro for popping the top value from a pCParser_translation_unitStack
 */
#define pCParser_translation_unitPop()  SCOPE_TOP(translation_unit) = ctx->pCParser_translation_unitStack->pop(ctx->pCParser_translation_unitStack)
/* ruleAttributeScopeFuncMacro(scope)
 */
/** Macro for popping the top value from a pCParser_declarationStack
 */
#define pCParser_declarationPop()  SCOPE_TOP(declaration) = ctx->pCParser_declarationStack->pop(ctx->pCParser_declarationStack)
/* ruleAttributeScopeFuncMacro(scope)
 */
/** Macro for popping the top value from a pCParser_init_declaratorStack
 */
#define pCParser_init_declaratorPop()  SCOPE_TOP(init_declarator) = ctx->pCParser_init_declaratorStack->pop(ctx->pCParser_init_declaratorStack)
/* ruleAttributeScopeFuncMacro(scope)
 */
/** Macro for popping the top value from a pCParser_initializerStack
 */
#define pCParser_initializerPop()  SCOPE_TOP(initializer) = ctx->pCParser_initializerStack->pop(ctx->pCParser_initializerStack)



/** Context tracking structure for CParser
 */
typedef struct CParser_Ctx_struct
{
    /** Built in ANTLR3 context tracker contains all the generic elements
     *  required for context tracking.
     */
    pANTLR3_PARSER   pParser;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  translation_unit stack for use by pCParser_translation_unitPush()
     *  and pCParser_translation_unitPop()
     */
    pANTLR3_STACK pCParser_translation_unitStack;
    pCParser_translation_unit_SCOPE   (*pCParser_translation_unitPush)(struct CParser_Ctx_struct * ctx);
    pCParser_translation_unit_SCOPE   pCParser_translation_unitTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  declaration stack for use by pCParser_declarationPush()
     *  and pCParser_declarationPop()
     */
    pANTLR3_STACK pCParser_declarationStack;
    pCParser_declaration_SCOPE   (*pCParser_declarationPush)(struct CParser_Ctx_struct * ctx);
    pCParser_declaration_SCOPE   pCParser_declarationTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  init_declarator stack for use by pCParser_init_declaratorPush()
     *  and pCParser_init_declaratorPop()
     */
    pANTLR3_STACK pCParser_init_declaratorStack;
    pCParser_init_declarator_SCOPE   (*pCParser_init_declaratorPush)(struct CParser_Ctx_struct * ctx);
    pCParser_init_declarator_SCOPE   pCParser_init_declaratorTop;
    /* ruleAttributeScopeDef(scope)
     */
    /** Pointer to the  initializer stack for use by pCParser_initializerPush()
     *  and pCParser_initializerPop()
     */
    pANTLR3_STACK pCParser_initializerStack;
    pCParser_initializer_SCOPE   (*pCParser_initializerPush)(struct CParser_Ctx_struct * ctx);
    pCParser_initializer_SCOPE   pCParser_initializerTop;


    pASTNodeList (*translation_unit)	(struct CParser_Ctx_struct * ctx, pASTBuilder b);
    pASTNode (*external_declaration)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*function_definition)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*declaration)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*declaration_specifier)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*declaration_specifier_list)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*init_declarator_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*init_declarator)	(struct CParser_Ctx_struct * ctx);
    void (*attribute_specifier)	(struct CParser_Ctx_struct * ctx);
    void (*attribute)	(struct CParser_Ctx_struct * ctx);
    void (*attribute_parameter_list)	(struct CParser_Ctx_struct * ctx);
    void (*attribute_parameter)	(struct CParser_Ctx_struct * ctx);
    void (*literal_constant_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_conditional_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_logical_or_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_logical_and_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_inclusive_or_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_exclusive_or_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_and_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_equality_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_relational_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_shift_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_additive_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_multiplicative_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_unary_expression)	(struct CParser_Ctx_struct * ctx);
    void (*literal_builtin_function)	(struct CParser_Ctx_struct * ctx);
    void (*literal_primary_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*storage_class_specifier)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*type_specifier)	(struct CParser_Ctx_struct * ctx);
    pASTTokenList (*builtin_type_list)	(struct CParser_Ctx_struct * ctx);
    CParser_builtin_type_return (*builtin_type)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*typeof_specification)	(struct CParser_Ctx_struct * ctx);
    void (*typeof_token)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*type_id)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*struct_or_union_specifier)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*struct_or_union)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*struct_declaration_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*struct_declaration)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*specifier_qualifier_list)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*struct_declarator_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*struct_declarator)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*enum_specifier)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*enumerator_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*enumerator)	(struct CParser_Ctx_struct * ctx);
    CParser_type_qualifier_return (*type_qualifier)	(struct CParser_Ctx_struct * ctx);
    pASTTokenList (*type_qualifier_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*declarator)	(struct CParser_Ctx_struct * ctx);
    pANTLR3_COMMON_TOKEN (*declarator_identifier)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*function_declarator)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*direct_declarator)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*declarator_suffix_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*declarator_suffix)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*pointer)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*parameter_type_list)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*parameter_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*parameter_declaration)	(struct CParser_Ctx_struct * ctx);
    pASTTokenList (*identifier_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*type_name)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*abstract_declarator)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*direct_abstract_declarator)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*abstract_declarator_suffix)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*initializer)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*initializer_list)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*argument_expression_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*additive_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*multiplicative_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*cast_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*unary_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_alignof)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_choose_expr)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_constant_p)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_expect)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_extract_return_addr)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_object_size)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_offsetof)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_prefetch)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_return_address)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_sizeof)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_types_compatible_p)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_va_arg)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_va_copy)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_va_end)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*builtin_function_va_start)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*postfix_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*postfix_expression_suffix_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*postfix_expression_suffix)	(struct CParser_Ctx_struct * ctx);
    CParser_unary_operator_return (*unary_operator)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*primary_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*constant)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*constant_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*assignment_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*designated_initializer_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*designated_initializer)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*lvalue)	(struct CParser_Ctx_struct * ctx);
    CParser_assignment_operator_return (*assignment_operator)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*conditional_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*logical_or_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*logical_and_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*inclusive_or_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*exclusive_or_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*and_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*equality_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*relational_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*shift_expression)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*statement)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*labeled_statement)	(struct CParser_Ctx_struct * ctx);
    void (*local_label_declaration)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*compound_statement)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*compound_braces_statement)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*declaration_or_statement_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*expression_statement)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*selection_statement)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*iteration_statement)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*jump_statement)	(struct CParser_Ctx_struct * ctx);
    void (*assembler_token)	(struct CParser_Ctx_struct * ctx);
    void (*assembler_register_variable)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*assembler_statement)	(struct CParser_Ctx_struct * ctx);
    pASTNodeList (*assembler_parameter_list)	(struct CParser_Ctx_struct * ctx);
    pASTNode (*assembler_parameter)	(struct CParser_Ctx_struct * ctx);
    pASTTokenList (*string_token_list)	(struct CParser_Ctx_struct * ctx);
    void (*string_list)	(struct CParser_Ctx_struct * ctx);
    void (*string_literals)	(struct CParser_Ctx_struct * ctx);
    void (*literal)	(struct CParser_Ctx_struct * ctx);
    void (*integer_literal)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred2)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred4)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred6)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred9)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred11)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred12)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred20)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred65)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred66)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred67)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred68)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred71)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred72)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred73)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred74)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred86)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred98)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred101)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred120)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred126)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred127)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred131)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred134)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred145)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred151)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred154)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred155)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred156)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred175)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred177)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred192)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred193)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred203)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred204)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred206)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred207)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred208)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred209)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred221)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred222)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred223)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred224)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred225)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred226)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred228)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred232)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred234)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred250)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred251)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred252)	(struct CParser_Ctx_struct * ctx);
    ANTLR3_BOOLEAN (*synpred254)	(struct CParser_Ctx_struct * ctx);    unsigned char * (*getGrammarFileName)();
    void	    (*free)   (struct CParser_Ctx_struct * ctx);
        
}
    CParser, * pCParser;

/* Function protoypes for the parser functions that external translation units
 * may wish to call.
 */
ANTLR3_API pCParser CParserNew         (pANTLR3_COMMON_TOKEN_STREAM     instream);/** Symbolic definitions of all the tokens that the parser will work with.
 * \{
 *
 * Antlr will define EOF, but we can't use that as it it is too common in
 * in C header files and that would be confusing. There is no way to filter this out at the moment
 * so we just undef it here for now. That isn't the value we get back from C recognizers
 * anyway. We are looking for ANTLR3_TOKEN_EOF.
 */
#ifdef	EOF
#undef	EOF
#endif
#ifdef	Tokens
#undef	Tokens
#endif 
#define LINE_COMMENT      23
#define FloatTypeSuffix      17
#define IntegerTypeSuffix      15
#define LETTER      11
#define STRING_GUTS      13
#define HexEscape      19
#define OCTAL_LITERAL      6
#define CHARACTER_LITERAL      8
#define Exponent      16
#define EOF      -1
#define HexDigit      14
#define WS      21
#define STRING_LITERAL      10
#define FLOATING_POINT_LITERAL      9
#define UnicodeEscape      20
#define IDENTIFIER      4
#define LINE_COMMAND      24
#define HEX_LITERAL      5
#define COMMENT      22
#define DECIMAL_LITERAL      7
#define EscapeSequence      12
#define OctalEscape      18
#ifdef	EOF
#undef	EOF
#define	EOF	ANTLR3_TOKEN_EOF
#endif

/* End of token definitions for CParser
 * =============================================================================
 */
/** \} */

#endif
/* END - Note:Keep extra linefeed to satisfy UNIX systems */
