/** ANSI C ANTLR v3 grammar

Adapted for C output target by Jim Idle - April 2007.

Translated from Jutta Degener's 1995 ANSI C yacc grammar by Terence Parr
July 2006.  The lexical rules were taken from the Java grammar.

Jutta says: "In 1985, Jeff Lee published his Yacc grammar (which
is accompanied by a matching Lex specification) for the April 30, 1985 draft
version of the ANSI C standard.  Tom Stockfisch reposted it to net.sources in
1987; that original, as mentioned in the answer to question 17.25 of the
comp.lang.c FAQ, can be ftp'ed from ftp.uu.net,
   file usenet/net.sources/ansi.c.grammar.Z.
I intend to keep this version as close to the current C Standard grammar as
possible; please let me know if you discover discrepancies. Jutta Degener, 1995"

Generally speaking, you need symbol table info to parse C; typedefs
define types and then IDENTIFIERS are either types or plain IDs.  I'm doing
the min necessary here tracking only type names.  This is a good example
of the global scope (called Symbols).  Every rule that declares its usage
of Symbols pushes a new copy on the stack effectively creating a new
symbol scope.  Also note rule declaration declares a rule scope that
lets any invoked rule see isTypedef boolean.  It's much easier than
passing that info down as parameters.  Very clean.  Rule
direct_declarator can then easily determine whether the IDENTIFIER
should be declared as a type name.

I have only tested this on a single file, though it is 3500 lines.

This grammar requires ANTLR v3 (3.0b8 or higher)

Terence Parr
July 2006

*/
grammar C;

options 
{
    backtrack = true;
    memoize = true;
    k = 2;
    language = C;
}

@lexer::includes
{
    #if !defined(__cplusplus) && ! defined(debugmsg)
        #define debugmsg(m, ...) printf(m, __VA_ARGS__)
    #endif
}

@parser::includes
{
    #include <assert.h>
    #include <ast_interface.h>
    #define BUILDER SCOPE_TOP(translation_unit)->builder
    
//    #define DEBUG 1
     
    #if (DEBUG == 1) && !defined(__cplusplus)
        #if !defined(debugmsg)
            #define debuginit() printf("(\%s:\%d) Executing INIT in \%s, BACKTRACKING is \%d\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, BACKTRACKING)
            #define debugafter() printf("(\%s:\%d) Executing AFTER in \%s, BACKTRACKING is \%d\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, BACKTRACKING)
            #define debugmsg(m, ...) printf("(\%s:\%d) " m, __FILE__, __LINE__, __VA_ARGS__)
            #define debugerr(m, ...) fprintf(stderr, "(\%s:\%d) " m, __FILE__, __LINE__, __VA_ARGS__)
        #endif
    #else
        #define debugmsg(...)
        #define debugerr(...)
        #define debuginit()
        #define debugafter()
    #endif

}

// @members inserts functions in C output file (parser without other
//          qualification. @lexer::members inserts functions in the lexer.
//
// In general do not use this too much (put in the odd tiny function perhaps),
// but include the generated header files in your own header and use this in
// separate translation units that contain support functions.
//
//@members 
//{
//}

translation_unit [pASTBuilder b] returns [pASTNodeList list]
    scope
    {
        pASTBuilder builder;
    }
    @init 
    {
        $translation_unit::builder = $b;
        pASTNodeList tail = 0; 
        $list = 0;
        
        // The entire translation_unit (file) is a scope
        if (!scopeCurrent(BUILDER)) {
//            debugmsg("Creating scope\n", 0);
	        scopePush(BUILDER, 0);
	    }
    }
    @after
    {
        if (BACKTRACKING == 0 && SCOPE_TOP(translation_unit))
            scopePop(BUILDER);
    }
    : (e=external_declaration 
        { 
            if ($e.node) {
	            tail = new_ASTNodeList(BUILDER, $e.node, tail); 
	            if (!$list) $list = tail;
            } 
//            debugmsg("Parsing line \%lu\n", LT(-1)->line);
        } 
      )*
    ;

external_declaration returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) {            
            INIT_NODE2($node, parent_node(BUILDER), nt_external_declaration);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
	        FINALIZE_NODE($node); 
	        pop_parent_node(BUILDER);
//	        debugmsg("Parsed line \%lu\n", LT(-1)->line);
	    } 
    }
    // Guide the parser with characteristic semantic predicates for the rules 
    : (assembler_token)=> a=assembler_statement
        { 
            $node->u.external_declaration.assembler_statement = $a.node; 
        }
    | ('__extension__'? 'typedef')=> d=declaration
        { 
            $node->u.external_declaration.declaration = $d.node; 
        }
    | ( declaration_specifier declarator attribute_specifier* '{' )=> 
        fd=function_definition
        { 
            $node->u.external_declaration.function_definition = $fd.node;
        }
    | d=declaration
        { 
            $node->u.external_declaration.declaration = $d.node; 
        }
    ;

function_definition returns [pASTNode node]     
    @init 
    {
        if (BACKTRACKING == 0) {                        
            INIT_NODE2($node, parent_node(BUILDER), nt_function_definition);
            push_parent_node(BUILDER, $node);

            // put parameters and locals into same scope for now
            scopePush(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
            scopePop(BUILDER);
        } 
    }
    : ds=declaration_specifier d=declarator attribute_specifier* cs=compound_statement // ANSI style
        {
            $node->u.function_definition.declaration_specifier = $ds.node;
            $node->u.function_definition.declarator = $d.node;
            $node->u.function_definition.compound_statement = $cs.node;
        }
    ;

//declaration_list returns [pASTNodeList list]
//    @init  { pASTNodeList tail = 0; $list = 0; }
//    : ( de=declaration 
//        { 
//            tail = new_ASTNodeList(BUILDER, $de.node, tail); 
//            if (!$list) $list = tail;  
//        }
//      )+
//    ;
    
declaration returns [pASTNode node]
    scope 
    {
        ANTLR3_BOOLEAN isTypedef;
        pASTNode typedefNode;
    }
    @init 
    {
        $declaration::isTypedef = ANTLR3_FALSE;
        $declaration::typedefNode = 0;
        
        if (BACKTRACKING == 0) {            
            INIT_NODE2($node, parent_node(BUILDER), nt_declaration);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( '__extension__'? 'typedef' ds=declaration_specifier? attribute_specifier*
    	{ 
    		$declaration::isTypedef = ANTLR3_TRUE; 
    	    $declaration::typedefNode = $node;
    	    $node->u.declaration.declaration_specifier = $ds.node;
    	}
        idl=init_declarator_list ';' // special case, looking for typedef
      | ds=declaration_specifier { $node->u.declaration.declaration_specifier = $ds.node; } 
          idl=init_declarator_list? ';'
      )
        {
            $node->u.declaration.init_declarator_list = $idl.list;
            $node->u.declaration.isTypedef = $declaration::isTypedef;
        }
      | ';'
    ;

declaration_specifier returns [pASTNode node]
    @init
    { 
        if (BACKTRACKING == 0) {            
            INIT_NODE2($node, parent_node(BUILDER), nt_declaration_specifier);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : dsl=declaration_specifier_list
        {
            $node->u.declaration_specifier.declaration_specifier_list = $dsl.list;
        }
    ;
    
declaration_specifier_list returns [pASTNodeList list]
    @init { pASTNodeList tail = 0; $list = 0; }
    :        
      ( sc=storage_class_specifier  
          { tail = new_ASTNodeList(BUILDER, $sc.node, tail); if (!$list) $list = tail; }
      | tq=type_qualifier           
          { tail = new_ASTNodeList(BUILDER, $tq.node, tail); if (!$list) $list = tail; }
      | attribute_specifier
      )*
      ts=type_specifier           
          { tail = new_ASTNodeList(BUILDER, $ts.node, tail); if (!$list) $list = tail; }
      ( sc=storage_class_specifier  
          { tail = new_ASTNodeList(BUILDER, $sc.node, tail); if (!$list) $list = tail; }
      | tq=type_qualifier           
          { tail = new_ASTNodeList(BUILDER, $tq.node, tail); if (!$list) $list = tail; }
      | attribute_specifier
      )*
    ;

init_declarator_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : i=init_declarator { $list = tail = new_ASTNodeList(BUILDER, $i.node, 0); } 
      (',' i=init_declarator { tail = new_ASTNodeList(BUILDER, $i.node, tail); } )*
    ;

init_declarator returns [pASTNode node] 
    scope 
    {
        ANTLR3_BOOLEAN isFuncTypedef;
        ANTLR3_BOOLEAN typedefNameFound;
    }
    @init  
    {
        pANTLR3_COMMON_TOKEN id;
        pASTNode d_decl;
        pASTNodeList l;
        
        if (BACKTRACKING == 0) {
            $init_declarator::isFuncTypedef = ANTLR3_FALSE;
            $init_declarator::typedefNameFound = ANTLR3_FALSE;
            
	        INIT_NODE2($node, parent_node(BUILDER), nt_init_declarator); 
	        push_parent_node(BUILDER, $node);
	    }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : d=declarator attribute_specifier* assembler_register_variable? ('=' i=initializer)?
        {
            $node->u.init_declarator.declarator = $d.node;
            $node->u.init_declarator.initializer  = $i.node;

            // See if we have to add this as a variable definition to the scope
            if ($i.node && 
                $node->parent->type == nt_declaration &&
                (!SCOPE_TOP(declaration) || !$declaration::isTypedef) &&
                (d_decl = $d.node->u.declarator.direct_declarator))
            {
                if (!d_decl->u.direct_declarator.identifier &&
                    d_decl->u.direct_declarator.declarator) {
                    d_decl = d_decl->u.direct_declarator.declarator
                                    ->u.declarator.direct_declarator;
                }
                id = d_decl ? d_decl->u.direct_declarator.identifier : 0;

                if (id) {
                    // This is a variable definition
                    scopeAddSymbol2(BUILDER, id->getText(id), stVariableDef,
                        d_decl, $node->parent->scope);
                }
            }
        }
    ;
    
attribute_specifier
    : ('__attribute__' | '__attribute') '(' '(' (attribute (',' attribute)*)? ')' ')'
    ;

attribute
    : 'const' 
    | '__const__'
    | IDENTIFIER ('(' attribute_parameter_list? ')')?
    ;
    
attribute_parameter_list
    : attribute_parameter (',' attribute_parameter)*
    ;

attribute_parameter
//    : constant_expression
    : literal_constant_expression
    ;
    
literal_constant_expression
    : literal_conditional_expression
    ;

literal_conditional_expression
    : literal_logical_or_expression ('?' literal_logical_or_expression ':' literal_conditional_expression)?
    ;

literal_logical_or_expression
    : literal_logical_and_expression ('||' literal_logical_and_expression)*
    ;

literal_logical_and_expression
    : literal_inclusive_or_expression ('&&' literal_inclusive_or_expression)*
    ;

literal_inclusive_or_expression
    : literal_exclusive_or_expression ('|' literal_exclusive_or_expression)*
    ;

literal_exclusive_or_expression
    : literal_and_expression ('^' literal_and_expression)*
    ;

literal_and_expression
    : literal_equality_expression ('&' literal_equality_expression)*
    ;

literal_equality_expression
    : literal_relational_expression (('=='|'!=') literal_relational_expression)*
    ;

literal_relational_expression
    : literal_shift_expression (('<'|'>'|'<='|'>=') literal_shift_expression)*
    ;

literal_shift_expression     
    : literal_additive_expression (('<<'|'>>') literal_additive_expression)*
    ;

literal_additive_expression
    : literal_multiplicative_expression (('+'|'-') literal_multiplicative_expression )*
    ;

literal_multiplicative_expression
    : literal_unary_expression (('*'|'/'|'%') literal_unary_expression )*
    ;

literal_unary_expression
    : literal_primary_expression
    | unary_operator literal_unary_expression
    | literal_builtin_function
    ;

literal_builtin_function
    : 'sizeof' '(' type_name ')'
    | '__alignof__' '(' type_name ')'
//    | '__builtin_offsetof' '(' type_name ',' pe=postfix_expression ')'
//    | '__builtin_va_arg' '(' ae=assignment_expression ',' t=type_name ')'
    | '__builtin_types_compatible_p' '(' type_name ',' type_name ')'
    ;

literal_primary_expression
    : '.'? IDENTIFIER
    | literal
    | '(' literal_constant_expression ')'        
    ;

storage_class_specifier returns [pASTNode node] 
    @init  
    { 
        if (BACKTRACKING == 0) {            
            INIT_NODE2($node, parent_node(BUILDER), nt_storage_class_specifier);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( t='extern'
	  | t='static'
	  | t='auto'
	  | t='register'
      | t='inline' | t='__inline'| t='__inline__'
	  )
	    {
	       $node->u.storage_class_specifier.token = $t;
	    }
    ;

type_specifier returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) {            
            INIT_NODE2($node, parent_node(BUILDER), nt_type_specifier);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    // Guide the parser with semantic predicates
    : (typeof_token)=>    ts=typeof_specification      { $node->u.type_specifier.typeof_specification = $ts.node; }
    | (struct_or_union)=> su=struct_or_union_specifier { $node->u.type_specifier.struct_or_union_specifier = $su.node; }
    | ('enum')=>          es=enum_specifier            { $node->u.type_specifier.enum_specifier = $es.node; }
    | (builtin_type)=>    bt=builtin_type_list         { $node->u.type_specifier.builtin_type_list = $bt.list; }
    | ti=type_id                                       { $node->u.type_specifier.type_id = $ti.node; }
    ;
    
builtin_type_list returns [pASTTokenList list]
    @init { pASTTokenList tail = 0; $list = 0; }
      // Allow a "const" in between built-in types, e.g., "long const int"
    : ( bt=builtin_type attribute_specifier? type_qualifier+ 
        bt2=builtin_type attribute_specifier?
        { 
          tail = new_ASTTokenList(BUILDER, $bt.start, tail); 
          if (!$list) $list = tail;
          tail = new_ASTTokenList(BUILDER, $bt2.start, tail);
        } 
      | bt=builtin_type attribute_specifier?
        { tail = new_ASTTokenList(BUILDER, $bt.start, tail); if (!$list) $list = tail; }
      )+
    ;

builtin_type
    : 'void'
    | 'char'
    | 'short'
    | 'int'
    | 'long'
    | 'float'
    | 'double'
    | 'signed' 
    | '__signed__'            // GCC extension
    | 'unsigned'
    | '_Bool'                 // GCC extension
    | '__builtin_va_list'     // GCC extension
    ;

typeof_specification returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) {            
            INIT_NODE2($node, parent_node(BUILDER), nt_typeof_specification);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : typeof_token '(' (pd=parameter_declaration | ae=assignment_expression) ')'
        {
            $node->u.typeof_specification.assignment_expression = $ae.node; 
            $node->u.typeof_specification.parameter_declaration = $pd.node; 
        }
    ;

typeof_token
    : 'typeof' 
    | '__typeof' 
    | '__typeof__'
    ;

type_id returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_type_id);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    :  { isTypeName(BUILDER, LT(1)->getText(LT(1))) }? id=IDENTIFIER
	    {
	        $node->u.type_id.identifier = $id;
	    }
    ;
    
struct_or_union_specifier returns [pASTNode node] 
    @init 
    {
        if (BACKTRACKING == 0) {             
            INIT_NODE2($node, parent_node(BUILDER), nt_struct_or_union_specifier);
            push_parent_node(BUILDER, $node);
            // structs are scopes
            scopePush(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
            scopePop(BUILDER);
        } 
    }
    : su=struct_or_union attribute_specifier*
      ( '{' sdl=struct_declaration_list? '}' 
            { $node->u.struct_or_union_specifier.isDefinition = ANTLR3_TRUE; }
      | id=IDENTIFIER 
            { scopeAddSymbol(BUILDER, $id.text, stStructOrUnionDef, $node); } 
        '{' sdl=struct_declaration_list? '}'
            { $node->u.struct_or_union_specifier.isDefinition = ANTLR3_TRUE; }
      | id=IDENTIFIER 
            { scopeAddSymbol(BUILDER, $id.text, stStructOrUnionDecl, $node); }
      )
        {
            $node->u.struct_or_union_specifier.struct_or_union = $su.node;            
            $node->u.struct_or_union_specifier.identifier = $id;            
            $node->u.struct_or_union_specifier.struct_declaration_list = $sdl.list;
        }
    ;

struct_or_union returns [pASTNode node] 
    @init  
    { 
         if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
         } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : 'struct' { $node->type = nt_struct_or_union_struct; }
    | 'union'  { $node->type = nt_struct_or_union_union; }
    ;

struct_declaration_list returns [pASTNodeList list]
    @init { pASTNodeList tail = 0; $list = 0; }
    : ( sd=struct_declaration { tail = new_ASTNodeList(BUILDER, $sd.node, tail); if (!$list) $list = tail; }
      | ';'
      )+
    ;

struct_declaration returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_struct_declaration);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : attribute_specifier* sql=specifier_qualifier_list attribute_specifier* sdl=struct_declarator_list? ';'
        {
            $node->u.struct_declaration.specifier_qualifier_list = $sql.list;
            $node->u.struct_declaration.struct_declarator_list = $sdl.list;
        }
    ;

specifier_qualifier_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : ( tq=type_qualifier { tail = new_ASTNodeList(BUILDER, $tq.node, tail); if (!$list) $list = tail; } 
      )* 
      ts=type_specifier { tail = new_ASTNodeList(BUILDER, $ts.node, tail); if (!$list) $list = tail; }
      ( attribute_specifier
      | tq=type_qualifier { tail = new_ASTNodeList(BUILDER, $tq.node, tail); if (!$list) $list = tail; } 
      )*
    ;

struct_declarator_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : sd=struct_declarator attribute_specifier* { $list = tail = new_ASTNodeList(BUILDER, $sd.node, 0); } 
      (',' sd=struct_declarator { tail = new_ASTNodeList(BUILDER, $sd.node, tail); } )*
    ;

struct_declarator returns [pASTNode node]
    @init  
    { 
         if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_struct_declarator);
            push_parent_node(BUILDER, $node);
         } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( d=declarator (':' ce=constant_expression)? 
      | ':' ce=constant_expression
      )
        {
            $node->u.struct_declarator.declarator = $d.node;
            $node->u.struct_declarator.constant_expression = $ce.node;
        }
    ;

enum_specifier returns [pASTNode node] 
    options 
    {
        k=3;
    }
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_enum_specifier);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : 'enum' attribute_specifier*
      ( '{' el=enumerator_list '}'
	  | id=IDENTIFIER '{' el=enumerator_list '}' { scopeAddSymbol(BUILDER, $id.text, stEnumDef, $node); }
	  | id=IDENTIFIER                            { scopeAddSymbol(BUILDER, $id.text, stEnumDecl, $node); }
	  )
	   {
	       $node->u.enum_specifier.identifier = $id;
	       $node->u.enum_specifier.enumerator_list = $el.list;      
	   }
    ;

enumerator_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : e=enumerator { $list = tail = new_ASTNodeList(BUILDER, $e.node, 0); } 
      (',' e=enumerator { tail = new_ASTNodeList(BUILDER, $e.node, tail); } )*
      ','?
    ;

enumerator returns [pASTNode node] 
    @init 
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_enumerator);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : id=IDENTIFIER ('=' ce=constant_expression)?
        {
            $node->u.enumerator.identifier = $id;            
            $node->u.enumerator.constant_expression = $ce.node;
            
            scopeAddSymbol(BUILDER, $id.text, stEnumerator, $node);
        }
    ;

type_qualifier returns [pASTNode node]
    @init 
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_type_qualifier);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( t='const'
      | t='__const__'
      | t='volatile'
      | t='__volatile__'
      ) { $node->u.type_qualifier.token = $t; }
    ;

type_qualifier_list returns [pASTTokenList list]
    @init  { pASTTokenList tail = 0; $list = 0; }
    : ( tq=type_qualifier { tail = new_ASTTokenList(BUILDER, $tq.start, tail); if (!$list) $list = tail; } )+
    ;
 
declarator returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_declarator);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( p=pointer attribute_specifier* dd=direct_declarator
      | attribute_specifier* dd=direct_declarator
//      | p=pointer
      )
        {
            $node->u.declarator.pointer = $p.node;
            $node->u.declarator.direct_declarator = $dd.node;
        }
    ;

declarator_identifier returns [pANTLR3_COMMON_TOKEN token]
    : id=IDENTIFIER attribute_specifier*
        {
            $token = $id;
            // Only add IDENTIFIER to typedef list if either:
            // - this is a non-function typedef or
            // - this is a function typedef and its name hasn't been parsed yet
            // This ensures that parameter names are not treated as typedef names
            if (SCOPE_TOP(declaration) && $declaration::isTypedef &&
                (!SCOPE_TOP(init_declarator) || 
                 !$init_declarator::isFuncTypedef || 
                 !$init_declarator::typedefNameFound))
            {
                scopeAddSymbol(BUILDER, $id.text, stTypedef, $declaration::typedefNode);
                if (SCOPE_TOP(init_declarator))
                    $init_declarator::typedefNameFound = ANTLR3_TRUE;
            }
        }
    ;

function_declarator returns [pASTNode node]
    : '('
        {
            // Tell following rules that this is a function typedef
            if (SCOPE_TOP(init_declarator) && SCOPE_TOP(declaration) && 
                $declaration::isTypedef)
            {
                $init_declarator::isFuncTypedef = ANTLR3_TRUE;
            }
        }        
      de=declarator // The first star is optional for GCC  
        { 
            $node = $de.node;
            $node->u.declarator.isFuncDeclarator = ANTLR3_TRUE; 
        } 
      ')'
    ;

direct_declarator returns [pASTNode node]
    @init  
    {
        pASTNode p;
        pASTNodeList l;
        pANTLR3_STRING id = 0;
        ANTLR3_UINT64 id_line = 0;

        if (BACKTRACKING == 0) {
            INIT_NODE2($node, parent_node(BUILDER), nt_direct_declarator);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        }
    }
    : (di=declarator_identifier | de=function_declarator) dsl=declarator_suffix_list?        
        {
            if ($di.token) {
                id = $di.token->getText($di.token);
                id_line = $di.token->line;
                $node->u.direct_declarator.identifier = $di.token;                
//                debugmsg("direct_declarator \"\%s\" at line\%lu\n", id->chars, $node->start->line); 
            }
            $node->u.direct_declarator.declarator = $de.node;
            $node->u.direct_declarator.declarator_suffix_list = $dsl.list;

            if (id && (!SCOPE_TOP(declaration) || !$declaration::isTypedef)) {
                // Try to find to which type this identifier belongs
                for (p = $node; p; p = p->parent) {
                    if (p->type == nt_function_definition) {
                        scopeAddSymbol2(BUILDER, id, stFunctionDef, $node, p->scope);
                        break;                            
                    }
                    // Either a function-type or non-function type variable                          
                    else if (p->type == nt_declaration) {
                        // Find end of the list
                        for (l = $dsl.list; l && l->next; l = l->next);
                        // Do we have a declarator suffix with parens?
                        if (l && l->item->type == nt_declarator_suffix_parens)
                            scopeAddSymbol2(BUILDER, id, stFunctionDecl, $node, p->scope);
                        else {
                            // Initializer is not yet set, so we only add a 
                            // stVariableDecl here (might be replaced by a
                            // stVariableDef later on in init_declarator, if 
                            // initializer exists)
                            scopeAddSymbol2(BUILDER, id, stVariableDecl, $node, p->scope);
                        }
                        break;                            
                    }
                    else if (p->type == nt_struct_declarator) {
                        scopeAddSymbol2(BUILDER, id, stStructMember, $node, p->scope);
                        break;                            
                    }
                    else if (p->type == nt_parameter_declaration) {
                        if (p->parent->parent->parent->parent->parent->type == nt_function_definition)
                        {
                            scopeAddSymbol2(BUILDER, id, stFunctionParam, $node, p->scope);
                        }
                        else if (p->parent->parent->parent->parent->parent->type == nt_init_declarator)
                        {
                            // Parameter name of function declaration, we're not interested                                                
                            debugmsg("Symbol \"\%s\" is ignored in line \%lu\n", id->chars, id_line);
                        }
                        else if (p->parent->parent->parent->parent->parent->type == nt_struct_declarator)
                        {
                            // Parameter name of function pointer declaration inside struct, we're not interested                                                
                            debugmsg("Symbol \"\%s\" is ignored in line \%lu\n", id->chars, id_line);
                        }
                        else if (p->parent->parent->parent->parent->parent->type == nt_parameter_declaration)
                        {
                            // Parameter name of function pointer declaration inside parameter list, we're not interested                                                
                            debugmsg("Symbol \"\%s\" is ignored in line \%lu\n", id->chars, id_line);
                        }
                        else if (p->parent->parent->parent->parent->parent->parent->type == nt_cast_expression)
                        {
                            // Parameter name of function pointer inside cast expression, we're not interested                                                
                            debugmsg("Symbol \"\%s\" is ignored in line \%lu\n", id->chars, id_line);
                        }
                        else {
                            debugerr("Unmatched identifier \"\%s\" in line \%lu\n", id->chars, id_line);
                        }
                        break;                   
                    }
                }
                
                if (!p)
                    debugerr("Unmatched identifier \"\%s\" in line \%lu\n", id->chars, id_line);
            }                    
        }
    ;

declarator_suffix_list returns [pASTNodeList list]
    @init { pASTNodeList tail = 0; $list = 0; }
    : (ds=declarator_suffix { tail = new_ASTNodeList(BUILDER, $ds.node, tail); if (!$list) $list = tail; } )+
    ;

declarator_suffix returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '[' ce=constant_expression? ']'
        {
            $node->type = nt_declarator_suffix_brackets;
            $node->u.declarator_suffix.constant_expression = $ce.node;
        }
    | '('
        {
            if (SCOPE_TOP(init_declarator) && SCOPE_TOP(declaration) && 
                $declaration::isTypedef)
                $init_declarator::isFuncTypedef = ANTLR3_TRUE;
        } 
        (ptl=parameter_type_list | il=identifier_list)? ')'
        {
            $node->type = nt_declarator_suffix_parens;
            $node->u.declarator_suffix.parameter_type_list = $ptl.node;
            $node->u.declarator_suffix.identifier_list = $il.list;
        }
    ;

pointer returns [pASTNode node] 
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_pointer);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '*'
      ( tql=type_qualifier_list p=pointer?
      | p=pointer
      | attribute_specifier*
      )
        {
            $node->u.pointer.type_qualifier_list = $tql.list;
            $node->u.pointer.pointer = $p.node;            
        }
    ;

parameter_type_list returns [pASTNode node]
    @init  
    {
        if (BACKTRACKING == 0) {
            INIT_NODE2($node, parent_node(BUILDER), nt_parameter_type_list);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : pl=parameter_list { $node->u.parameter_type_list.parameter_list = $pl.list; } 
      (',' '...' { $node->u.parameter_type_list.openArgs = 1; } )?
    ;

parameter_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : pd=parameter_declaration { $list = tail = new_ASTNodeList(BUILDER, $pd.node, 0); } 
      (',' pd=parameter_declaration { tail = new_ASTNodeList(BUILDER, $pd.node, tail); } )*
    ;

parameter_declaration returns [pASTNode node]
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_parameter_declaration);
            push_parent_node(BUILDER, $node);
        }
        pASTNodeList tail = 0; 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ds=declaration_specifier { $node->u.parameter_declaration.declaration_specifier = $ds.node; } 
      ( // TODO: Change type from list to node
        ( d=declarator            { tail = new_ASTNodeList(BUILDER, $d.node, tail); }
        | ad=abstract_declarator  { tail = new_ASTNodeList(BUILDER, $ad.node, tail); }
        ) 
        {
            if (!$node->u.parameter_declaration.declarator_list)
                 $node->u.parameter_declaration.declarator_list = tail;
        }
      )? // Why should we allow this multiple times???
    ;

identifier_list returns [pASTTokenList list]
    @init  { pASTTokenList tail = 0; $list = 0; }
    : id=IDENTIFIER { $list = tail = new_ASTTokenList(BUILDER, $id, 0); } 
      (',' id=IDENTIFIER { tail = new_ASTTokenList(BUILDER, $id, tail); } )*
    ;

type_name returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_type_name);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : sql=specifier_qualifier_list ad=abstract_declarator?
        {
            $node->u.type_name.specifier_qualifier_list = $sql.list;
            $node->u.type_name.abstract_declarator = $ad.node;
        }
    ;

abstract_declarator returns [pASTNode node]
    @init  
    {
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_abstract_declarator);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( p=pointer da=direct_abstract_declarator?
      | da=direct_abstract_declarator
      )
        {
            $node->u.abstract_declarator.pointer = $p.node;
            $node->u.abstract_declarator.direct_abstract_declarator = $da.node;  
//            debugerr("Found abstract declarator at line \%lu\n", $node->start->line);          
        }
    ;

direct_abstract_declarator returns [pASTNode node]
    @init  
    {
        pASTNodeList tail = 0;

        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_direct_abstract_declarator);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    // Here the star is NOT optional (as opposed to a direct_declarator)
    : ( '(' ad=abstract_declarator ')' | ads=abstract_declarator_suffix )
        {
            $node->u.direct_abstract_declarator.abstract_declarator = $ad.node;
            if ($ads.node)
                $node->u.direct_abstract_declarator.abstract_declarator_suffix_list = tail = new_ASTNodeList(BUILDER, $ads.node, 0);
        } 
      ( ads=abstract_declarator_suffix
        {
            tail = new_ASTNodeList(BUILDER, $ads.node, tail);
            if (!$node->u.direct_abstract_declarator.abstract_declarator_suffix_list)
                $node->u.direct_abstract_declarator.abstract_declarator_suffix_list = tail;
        }
      )*
    ;

abstract_declarator_suffix returns [pASTNode node]
    @init  
    { 
         if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '[' ce=constant_expression? ']'
        {
            $node->type = nt_abstract_declarator_suffix_brackets;
            $node->u.abstract_declarator_suffix.constant_expression = $ce.node;
        }
    |  '(' ptl=parameter_type_list? ')'
        {
            $node->type = nt_abstract_declarator_suffix_parens;
            $node->u.abstract_declarator_suffix.constant_expression = $ptl.node;
        }  
    ;

initializer returns [pASTNode node]
    scope {
        // Required to allow/disallow designated_initializers later on
        ANTLR3_BOOLEAN isInitializer;
    }
    @init  
    {
        $initializer::isInitializer = ANTLR3_TRUE;

        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_initializer);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ('{')=> 
        '{' (il=initializer_list ','?)? '}' { $node->u.initializer.initializer_list = $il.list; }
    | ae=assignment_expression { $node->u.initializer.assignment_expression = $ae.node; }
    ;

initializer_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : in=initializer  { $list = tail = new_ASTNodeList(BUILDER, $in.node, 0); } 
      (',' in=initializer { tail = new_ASTNodeList(BUILDER, $in.node, tail); } )*
    ;

// E x p r e s s i o n s

argument_expression_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : ae=assignment_expression { $list = tail = new_ASTNodeList(BUILDER, $ae.node, 0); } 
      (',' ae=assignment_expression { tail = new_ASTNodeList(BUILDER, $ae.node, tail); } )*
    ;

additive_expression returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_additive_expression);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=multiplicative_expression { $node->u.binary_expression.left = $l.node; }
      ((op='+'|op='-') r=multiplicative_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

multiplicative_expression returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_multiplicative_expression);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=cast_expression { $node->u.binary_expression.left = $l.node; }
      ((op='*'|op='/'|op='%') r=cast_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

cast_expression returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_cast_expression);
            push_parent_node(BUILDER, $node);
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '(' t=type_name ')' 
      ( ce=cast_expression
      | in=initializer
      )
        { 
            $node->u.cast_expression.type_name = $t.node;
            $node->u.cast_expression.cast_expression = $ce.node;
            $node->u.cast_expression.initializer = $in.node;
        }
    | ue=unary_expression
        { 
            $node->u.cast_expression.unary_expression = $ue.node;
        }
    ;

unary_expression returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_unary_expression);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : pe=postfix_expression
        {
            $node->u.unary_expression.postfix_expression = $pe.node;
        }
    | '++' ue=unary_expression
        {
            $node->type = nt_unary_expression_inc;
            $node->u.unary_expression.unary_expression = $ue.node;
        }
    | '--' ue=unary_expression
        {
            $node->type = nt_unary_expression_dec;
            $node->u.unary_expression.unary_expression = $ue.node;
        }
    | op=unary_operator ce=cast_expression
        {
            $node->type = nt_unary_expression_op;
            $node->u.unary_expression.unary_operator = $op.start;
            $node->u.unary_expression.cast_expression = $ce.node;
        }
    | bf=builtin_function
        {
            $node->type = nt_unary_expression_builtin;
            $node->u.unary_expression.builtin_function = $bf.node;
        }
    ;

builtin_function returns [pASTNode node]
    // See http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Other-Builtins.html
    : ( n=builtin_function_alignof
      | n=builtin_function_choose_expr
      | n=builtin_function_constant_p
	  | n=builtin_function_expect
	  | n=builtin_function_extract_return_addr
	  | n=builtin_function_object_size
	  | n=builtin_function_offsetof
	  | n=builtin_function_prefetch
	  | n=builtin_function_return_address
	  | n=builtin_function_sizeof
	  | n=builtin_function_types_compatible_p
	  | n=builtin_function_va_arg
	  | n=builtin_function_va_copy
	  | n=builtin_function_va_end
	  | n=builtin_function_va_start
	  )
        {
            $node = $n.node;
	    }    
    ;
        
builtin_function_alignof returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_alignof);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__alignof__' (ue=unary_expression | '(' t=type_name ')')
        {
            $node->u.builtin_function_alignof.unary_expression = $ue.node;
            $node->u.builtin_function_alignof.type_name = $t.node;
        }
    ;
        
builtin_function_choose_expr returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_choose_expr);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_choose_expr' '(' ce=constant_expression  ',' 
        ae1=assignment_expression ',' ae2=assignment_expression ')'
        {
            $node->u.builtin_function_choose_expr.constant_expression = $ce.node;
            $node->u.builtin_function_choose_expr.assignment_expression1 = $ae1.node;
            $node->u.builtin_function_choose_expr.assignment_expression2 = $ae2.node;
        }
    ;

builtin_function_constant_p returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_constant_p);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_constant_p' ue=unary_expression
        {
            $node->u.builtin_function_constant_p.unary_expression = $ue.node;
        }
    ;
        
builtin_function_expect returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_expect);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_expect' '(' ae=assignment_expression ',' c=constant ')'
        {
            $node->u.builtin_function_expect.assignment_expression = $ae.node;
            $node->u.builtin_function_expect.constant = $c.node;
        }
    ;
        
builtin_function_extract_return_addr returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_extract_return_addr);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_extract_return_addr' '(' ae=assignment_expression ')'
        {
            $node->u.builtin_function_extract_return_addr.assignment_expression = $ae.node;
        }
    ;
        
builtin_function_object_size returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_object_size);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_object_size' '(' ae=assignment_expression ',' c=constant ')'
        {
            $node->u.builtin_function_object_size.assignment_expression = $ae.node;
            $node->u.builtin_function_object_size.constant = $c.node;
        }
    ;
        
builtin_function_offsetof returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_offsetof);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_offsetof' '(' t=type_name ',' pe=postfix_expression ')'
        {
            $node->u.builtin_function_offsetof.type_name = $t.node;
            $node->u.builtin_function_offsetof.postfix_expression = $pe.node;
        }
    ;
        
builtin_function_prefetch returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_prefetch);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_prefetch' '(' ae=assignment_expression 
        (',' ce1=constant_expression ',' ce2=constant_expression )? ')'
        {
            $node->u.builtin_function_prefetch.assignment_expression = $ae.node;
            $node->u.builtin_function_prefetch.constant_expression1 = $ce1.node;
            $node->u.builtin_function_prefetch.constant_expression2 = $ce2.node;
        }
    ;
        
builtin_function_return_address returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_return_address);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_return_address' '(' c=constant ')'
        {
            $node->u.builtin_function_return_address.constant = $c.node;
        }
    ;

builtin_function_sizeof returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_sizeof);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : 'sizeof' (ue=unary_expression | '(' t=type_name ')')
        {
            $node->u.builtin_function_sizeof.unary_expression = $ue.node;
            $node->u.builtin_function_sizeof.type_name = $t.node;
        }
    ;

builtin_function_types_compatible_p returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_types_compatible_p);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_types_compatible_p' '(' t=type_name ',' t2=type_name ')'
        {
            $node->u.builtin_function_types_compatible_p.type_name1 = $t.node;
            $node->u.builtin_function_types_compatible_p.type_name2 = $t2.node;
        }
    ;
        
builtin_function_va_arg returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_va_arg);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_va_arg' '(' ae=assignment_expression ',' t=type_name ')'
        {
            $node->u.builtin_function_va_xxx.assignment_expression = $ae.node;
            $node->u.builtin_function_va_xxx.type_name = $t.node;
        }
    ;
        
builtin_function_va_copy returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_va_copy);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_va_copy' '(' ae1=assignment_expression ',' ae2=assignment_expression ')'
        {
            $node->u.builtin_function_va_xxx.assignment_expression = $ae1.node;
            $node->u.builtin_function_va_xxx.assignment_expression2 = $ae2.node;
        }
    ;

builtin_function_va_end returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_va_end);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_va_end' '(' ae=assignment_expression ')'
        {
            $node->u.builtin_function_va_xxx.assignment_expression = $ae.node;
        }
    ;

builtin_function_va_start returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_builtin_function_va_start);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '__builtin_va_start' '(' ae1=assignment_expression ',' ae2=assignment_expression ')'
        {
            $node->u.builtin_function_va_xxx.assignment_expression = $ae1.node;
            $node->u.builtin_function_va_xxx.assignment_expression2 = $ae2.node;
        }
    ;


postfix_expression returns [pASTNode node]
	@init  
	{ 
	   if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_postfix_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : pe=primary_expression           
      pl=postfix_expression_suffix_list?
        {
          $node->u.postfix_expression.primary_expression = $pe.node;
          $node->u.postfix_expression.postfix_expression_suffix_list = $pl.list;
        }
    ;

postfix_expression_suffix_list returns [pASTNodeList list]
    @init { pASTNodeList tail = 0; $list = 0; }
    : (
        pes=postfix_expression_suffix
        {
            tail = new_ASTNodeList(BUILDER, $pes.node, tail);
            if (!$list) $list = tail;
        }
      )+
    ;
    
postfix_expression_suffix returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '[' ex=expression ']'                 { $node->type = nt_postfix_expression_brackets; $node->u.postfix_expression_suffix.expression = $ex.list; }
    | '(' ')'                               { $node->type = nt_postfix_expression_parens; }
    | '(' ael=argument_expression_list ')'  { $node->type = nt_postfix_expression_parens; $node->u.postfix_expression_suffix.argument_expression_list = $ael.list; }
    |   ( '.' id=IDENTIFIER                 { $node->type = nt_postfix_expression_dot; }
//        | '*' id=IDENTIFIER                 { $node->type = nt_postfix_expression_star; }
        | '->' id=IDENTIFIER                { $node->type = nt_postfix_expression_arrow; }
        ) { $node->u.postfix_expression_suffix.identifier = $id; }
    | '++'                                  { $node->type = nt_postfix_expression_inc; }
    | '--'                                  { $node->type = nt_postfix_expression_dec; }
    ;

unary_operator
    : '&'
    | '&&'
    | '*'
    | '+'
    | '-'
    | '~'
    | '!'
    ;

primary_expression returns [pASTNode node]
	@init  
	{ 
	   if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_primary_expression);
            push_parent_node(BUILDER, $node);
        }
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ('(' '{')=> 
        cbs=compound_braces_statement { $node->u.primary_expression.compound_braces_statement = $cbs.node; }
    | ('(')=>
        '(' ex=expression ')'         { $node->u.primary_expression.expression = $ex.list; }
    | ('.')=> 
      '.' id=IDENTIFIER               { $node->u.primary_expression.identifier = $id; $node->u.primary_expression.hasDot = ANTLR3_TRUE; }
      // Actually we would need to test here if we have a declaration such as 
      // "int foo = foo;", however I can't convince ANTLR to put the first "foo"
      // into the symbol table before evaluating the primary_expression rule.
      // As last resort, we check if we are in an init_declarator rule. *sigh* 
    | //{ isSymbolName(BUILDER, LT(1)->getText(LT(1))) || SCOPE_TOP(init_declarator) }? 
        id=IDENTIFIER                 { $node->u.primary_expression.identifier = $id; $node->u.primary_expression.hasDot = ANTLR3_FALSE; }
    | co=constant                     { $node->u.primary_expression.constant = $co.node; }
    ;

constant returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( l=HEX_LITERAL               { $node->type = nt_constant_int; }
      | l=OCTAL_LITERAL             { $node->type = nt_constant_int; }
      | l=DECIMAL_LITERAL           { $node->type = nt_constant_int; }
      | l=CHARACTER_LITERAL         { $node->type = nt_constant_char; }
      | sl=string_token_list        { $node->type = nt_constant_string; }
      | l=FLOATING_POINT_LITERAL    { $node->type = nt_constant_float; }
      ) { 
            $node->u.constant.literal = $l; 
            $node->u.constant.string_token_list = $sl.list;
        }
    ;

/////

expression returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : ae=assignment_expression { $list = tail = new_ASTNodeList(BUILDER, $ae.node, 0); }
      (',' ae=assignment_expression { tail = new_ASTNodeList(BUILDER, $ae.node, tail); } )*
    ;

constant_expression returns [pASTNode node]
	@init  
	{ 
	   if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_constant_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ce=conditional_expression { $node->u.constant_expression.conditional_expression = $ce.node; }
    ;

assignment_expression returns [pASTNode node]
	@init  
	{ 
	   if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_assignment_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : lv=lvalue ao=assignment_operator ae=assignment_expression
        {
            $node->u.assignment_expression.lvalue = $lv.node;
            $node->u.assignment_expression.assignment_operator = $ao.start;
            $node->u.assignment_expression.assignment_expression = $ae.node;
        }
    | { SCOPE_TOP(initializer) && $initializer::isInitializer }?
      (lv=lvalue | dil=designated_initializer_list) op='=' in=initializer
        {
            $node->u.assignment_expression.lvalue = $lv.node;
            $node->u.assignment_expression.designated_initializer_list = $dil.list;
            $node->u.assignment_expression.assignment_operator = $op;
            $node->u.assignment_expression.initializer = $in.node;
        }
    | ce=conditional_expression
        {
            $node->u.assignment_expression.conditional_expression = $ce.node;
        }
    ;
    
designated_initializer_list returns [pASTNodeList list]
    @init { pASTNodeList tail = 0; $list = 0; }
    : ( di=designated_initializer { tail = new_ASTNodeList(BUILDER, $di.node, tail); if (!$list) $list = tail; } )+
    ;

designated_initializer returns [pASTNode node]
    @init  
    { 
       if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_designated_initializer);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ( '[' co1=constant_expression ']'
      | '[' co1=constant_expression '...' co2=constant_expression ']'
      )
        {
            $node->u.designated_initializer.constant_expression1 = $co1.node;
            $node->u.designated_initializer.constant_expression2 = $co2.node;
        }
    ;
    
lvalue returns [pASTNode node]
	@init  
	{ 
	   if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_lvalue);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : '(' t=type_name ')' l=lvalue
        {
            $node->u.lvalue.type_name = $t.node;
            $node->u.lvalue.lvalue = $l.node;
        }
    | ue=unary_expression { $node->u.lvalue.unary_expression = $ue.node; }
    ;

assignment_operator
    : '='
    | '*='
    | '/='
    | '%='
    | '+='
    | '-='
    | '<<='
    | '>>='
    | '&='
    | '^='
    | '|='
    ;

conditional_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_conditional_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : loe=logical_or_expression ('?' ex=expression? ':' ce=conditional_expression)?
        {
            $node->u.conditional_expression.logical_or_expression = $loe.node;
            $node->u.conditional_expression.expression = $ex.list;
            $node->u.conditional_expression.conditional_expression = $ce.node;
        }
    ;

logical_or_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_logical_or_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
	@after 
	{ 
	    if (BACKTRACKING == 0) {
	        FINALIZE_NODE($node);
	        pop_parent_node(BUILDER);
        } 
	}
    : l=logical_and_expression { $node->u.binary_expression.left = $l.node; }
      (op='||' r=logical_and_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

logical_and_expression returns [pASTNode node]
	@init  { 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_logical_and_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=inclusive_or_expression { $node->u.binary_expression.left = $l.node; }
      (op='&&' r=inclusive_or_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

inclusive_or_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_inclusive_or_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=exclusive_or_expression { $node->u.binary_expression.left = $l.node; }
      (op='|' r=exclusive_or_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

exclusive_or_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_exclusive_or_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=and_expression { $node->u.binary_expression.left = $l.node; }
      (op='^' r=and_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

and_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_and_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=equality_expression { $node->u.binary_expression.left = $l.node; }
      (op='&' r=equality_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

equality_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_equality_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=relational_expression { $node->u.binary_expression.left = $l.node; }
      ((op='=='|op='!=') r=relational_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

relational_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_relational_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node);
            pop_parent_node(BUILDER);
        }
    }
    : l=shift_expression { $node->u.binary_expression.left = $l.node; }
      ((op='<'|op='>'|op='<='|op='>=') r=shift_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

shift_expression returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_shift_expression);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : l=additive_expression { $node->u.binary_expression.left = $l.node; }
      ((op='<<'|op='>>') r=additive_expression { $node = pushdown_binary_expression(BUILDER, $op, $node, $r.node); } )*
    ;

// S t a t e m e n t s

statement returns [pASTNode node]
    : attribute_specifier*
      ( s=labeled_statement
      | s=compound_statement
      | s=expression_statement
      | s=selection_statement
      | s=iteration_statement
      | s=jump_statement
      | s=assembler_statement
      ) 
        {
            $node = $s.node; // Just pass this node through
        }
    ;

labeled_statement returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_labeled_statement);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : id=IDENTIFIER ':' st=statement
        {
            $node->u.labeled_statement.identifier = $id;
            $node->u.labeled_statement.statement = $st.node;
        }

    | 'case' ce=constant_expression ('...' ce2=constant_expression)? ':' st=statement
        {
            $node->type = nt_labeled_statement_case;
            $node->u.labeled_statement.constant_expression = $ce.node;
            $node->u.labeled_statement.constant_expression2 = $ce2.node;
            $node->u.labeled_statement.statement = $st.node;
        }        
    | 'default' ':' st=statement
        {
            $node->type = nt_labeled_statement_default;
            $node->u.labeled_statement.statement = $st.node;
        }
    ;

local_label_declaration
    : '__label__' IDENTIFIER (',' IDENTIFIER)* ';'
    ;

compound_statement returns [pASTNode node]
    @init 
    {
        if (BACKTRACKING == 0) {                
            INIT_NODE2($node, parent_node(BUILDER), nt_compound_statement);
            push_parent_node(BUILDER, $node);

            // blocks have a scope of symbols
            scopePush(BUILDER, $node);   
        }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
            scopePop(BUILDER);
        } 
    }
    : '{' local_label_declaration? dsl=declaration_or_statement_list? '}'
        {
            $node->u.compound_statement.declaration_or_statement_list = $dsl.list;
        }
    ;

compound_braces_statement returns [pASTNode node]
    @init 
    {
	     if (BACKTRACKING == 0) {               
            INIT_NODE2($node, parent_node(BUILDER), nt_compound_braces_statement);
            push_parent_node(BUILDER, $node);

            // blocks have a scope of symbols
            scopePush(BUILDER, $node);
         }
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
            scopePop(BUILDER);
        } 
    }
    : '(' '{' local_label_declaration? dsl=declaration_or_statement_list? '}' ')'
//    : '(' '{' local_label_declaration? dsl=declaration_list? sl=statement_list '}' ')'
        {
            $node->u.compound_braces_statement.declaration_or_statement_list = $dsl.list;
        }
    ;

declaration_or_statement_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    // Put the assembler_statement first so that it becomes a direct item of
    // declaration_or_statement_list instead of external_declaration
    : ( (assembler_token)=> a=assembler_statement
        { 
            tail = new_ASTNodeList(BUILDER, $a.node, tail); 
            if (!$list) $list = tail;  
//            debugmsg("Parsed \%s at line \%lu\n",
//                ast_node_type_to_str(tail->item),
//                tail->item->start->line); 
        }
      | ed=external_declaration  
        { 
            tail = new_ASTNodeList(BUILDER, $ed.node, tail); 
            if (!$list) $list = tail;  
//            debugmsg("Parsed \%s at line \%lu\n",
//                ast_node_type_to_str(tail->item),
//                tail->item->start->line); 
        }
      | st=statement    
        { 
            tail = new_ASTNodeList(BUILDER, $st.node, tail); 
            if (!$list) $list = tail;  
//            debugmsg("Parsed \%s at line \%lu\n",
//                ast_node_type_to_str(tail->item),
//                tail->item->start->line); 
        }
      )+
   ;

expression_statement returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_expression_statement);
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ';'
    | ex=expression ';' { $node->u.expression_statement.expression = $ex.list; }
    ;

selection_statement returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : 'if' '(' ex=expression ')' st1=statement (('else')=> 'else' st2=statement)?
        {
            $node->type = nt_selection_statement_if;
            $node->u.selection_statement.expression = $ex.list;
            $node->u.selection_statement.statement = $st1.node;
            $node->u.selection_statement.statement_else = $st2.node;
        }
    | 'switch' '(' ex=expression ')' st=statement
        {
            $node->type = nt_selection_statement_switch;
            $node->u.selection_statement.expression = $ex.list;
            $node->u.selection_statement.statement = $st.node;
        }
    ;

iteration_statement returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : 'while' '(' ex=expression ')' st=statement
        {
            $node->type = nt_iteration_statement_while;
            $node->u.iteration_statement.expression = $ex.list;
            $node->u.iteration_statement.statement = $st.node;
        }
    | 'do' st=statement 'while' '(' ex=expression ')' ';'
        {
            $node->type = nt_iteration_statement_do;
            $node->u.iteration_statement.expression = $ex.list;
            $node->u.iteration_statement.statement = $st.node;
        }
    | 'for' '(' es1=expression_statement es2=expression_statement ex=expression? ')' st=statement
        {
            $node->type = nt_iteration_statement_for;
            $node->u.iteration_statement.expression_statement_init = $es1.node;
            $node->u.iteration_statement.expression_statement_cond = $es2.node;
            $node->u.iteration_statement.expression = $ex.list;
            $node->u.iteration_statement.statement = $st.node;
        }
    ;
 
jump_statement returns [pASTNode node]
	@init  
	{ 
	    if (BACKTRACKING == 0) { 
            INIT_NODE($node, parent_node(BUILDER));
            push_parent_node(BUILDER, $node);
        } 
	}
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : 'goto' ue=unary_expression ';' { $node->type = nt_jump_statement_goto; $node->u.jump_statement.unary_expression = $ue.node; }
    | 'continue' ';'              { $node->type = nt_jump_statement_continue; }
    | 'break' ';'                 { $node->type = nt_jump_statement_break; }
    | 'return' ';'                { $node->type = nt_jump_statement_return; }
    | 'return' in=initializer ';' { $node->type = nt_jump_statement_return; $node->u.jump_statement.initializer = $in.node; }
    ;

assembler_token
    : 'asm' 
    | '__asm__'
    ;

assembler_register_variable
    : assembler_token '(' string_literals ')'
    ;

assembler_statement returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_assembler_statement);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : assembler_token tql=type_qualifier_list? '(' ins=string_token_list 
      (':' in=assembler_parameter_list? 
        (':' out=assembler_parameter_list?
          (':' string_list?)?)?)? 
      ')' ';'
        {
            $node->u.assembler_statement.type_qualifier_list = $tql.list;
            $node->u.assembler_statement.instructions = $ins.list;
            $node->u.assembler_statement.input_parameter_list = $in.list;
            $node->u.assembler_statement.output_parameter_list = $out.list;
        }
    ;

assembler_parameter_list returns [pASTNodeList list]
    @init  { pASTNodeList tail = 0; $list = 0; }
    : a=assembler_parameter { $list = tail = new_ASTNodeList(BUILDER, $a.node, 0); } 
      (',' a=assembler_parameter { tail = new_ASTNodeList(BUILDER, $a.node, tail); } )*
    ;
    
assembler_parameter returns [pASTNode node]
    @init  
    { 
        if (BACKTRACKING == 0) { 
            INIT_NODE2($node, parent_node(BUILDER), nt_assembler_parameter);
            push_parent_node(BUILDER, $node);
        } 
    }
    @after 
    {
        if (BACKTRACKING == 0) { 
            FINALIZE_NODE($node); 
            pop_parent_node(BUILDER);
        } 
    }
    : ('[' alias=IDENTIFIER ']')? constr=string_token_list '(' ae=assignment_expression ')'
        {
            $node->u.assembler_parameter.alias = $alias;
            $node->u.assembler_parameter.constraint_list = $constr.list;
            $node->u.assembler_parameter.assignment_expression = $ae.node;
        }
    ;

string_token_list returns [pASTTokenList list]
    @init  { pASTTokenList tail = 0; $list = 0; }
    : ( s=STRING_LITERAL { tail = new_ASTTokenList(BUILDER, $s, tail); if (!$list) $list = tail; } )+
    ;
    
string_list
    : string_literals (',' string_literals)*
    ;

string_literals
    : STRING_LITERAL+
    ;

literal
    : HEX_LITERAL             
    | OCTAL_LITERAL           
    | DECIMAL_LITERAL         
    | CHARACTER_LITERAL       
    | STRING_LITERAL+          
    | FLOATING_POINT_LITERAL  
    ;

integer_literal
    : DECIMAL_LITERAL
    | HEX_LITERAL
    | OCTAL_LITERAL
    ;

IDENTIFIER
    :   LETTER (LETTER|'0'..'9')*
    ;
    
fragment
LETTER
    :   '$'
    |   'A'..'Z'
    |   'a'..'z'
    |   '_'
    ;

CHARACTER_LITERAL
    :   '\'' ( EscapeSequence | ~('\''|'\\') ) '\''
    ;

STRING_LITERAL
    :  '"' STRING_GUTS '"'
    ;

fragment
STRING_GUTS :   ( EscapeSequence | ~('\\'|'"') )* ;

HEX_LITERAL : '0' ('x'|'X') HexDigit+ IntegerTypeSuffix? ;

DECIMAL_LITERAL : ('0' | '1'..'9' '0'..'9'*) IntegerTypeSuffix? ;

OCTAL_LITERAL : '0' ('0'..'7')+ IntegerTypeSuffix? ;

fragment
HexDigit : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
IntegerTypeSuffix
    :   ('l'|'L') ('l'|'L')?
    |   ('u'|'U')  (('l'|'L') ('l'|'L')?)?
    |   ('l'|'L') ('l'|'L')? ('u'|'U')
    ;

FLOATING_POINT_LITERAL
    :   ('0'..'9')+ '.' ('0'..'9')* Exponent? FloatTypeSuffix?
    |   '.' ('0'..'9')+ Exponent? FloatTypeSuffix?
    |   ('0'..'9')+ ( Exponent FloatTypeSuffix? | FloatTypeSuffix)
    ;

fragment
Exponent : ('e'|'E') ('+'|'-')? ('0'..'9')+ ;

fragment
FloatTypeSuffix : ('f'|'F'|'d'|'D') ;

fragment
EscapeSequence
    :   '\\' ('a'|'b'|'t'|'n'|'f'|'r'|'e'|'E'|'v'|'\"'|'\''|'\\')
    |   OctalEscape
    |   HexEscape
    |   UnicodeEscape
    ;

fragment
OctalEscape
    :   '\\' ('0'..'3') ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7')
    ;

fragment
HexEscape
    :   '\\' 'x' HexDigit+
    ;

fragment
UnicodeEscape
    :   '\\' 'u' HexDigit HexDigit HexDigit HexDigit
    |   '\\' 'U' HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit
    ;

WS  :  (' '|'\r'|'\t'|'\u000C'|'\n') {$channel=HIDDEN;}
    ;

COMMENT
    :   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    ;

LINE_COMMENT
    : '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    ;

// ignore #line info for now
LINE_COMMAND 
    : '#' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    ;
