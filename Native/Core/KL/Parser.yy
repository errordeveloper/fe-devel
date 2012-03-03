%define api.pure
%locations
%lex-param { KL::Context &context }
%parse-param { KL::Context &context }
%name-prefix="kl_"
%defines
%error-verbose
%debug

%expect 1

%code top {
#define YYDEBUG 1

// [pzion 20110919] Needed to avoid immediate "memory exhausted" error from Bison
// The Bison docs state that the token stack cannot grow on the the parser when
// compiled with a C++ compiler so this is actually the maximum depth as well.
// [jcgagnon 20110920] Bumped that down from 65536 to 16384. This is not the number
// of bytes, but the number of elements, which account for 26 bytes each.
// 65536 elements caused a stack alloc of more than 1703936 bytes, which blew the 
// stack limit of the plugin, who's stack size is decided by chrome and not by fabric.
#define YYINITDEPTH 16384
#define YYMAXDEPTH 16384

#define RTLOC (CG::Location( yyloc.filename, yyloc.first_line, yyloc.first_column ))
}

%code requires {
#include <Fabric/Base/RC/String.h>
#include <Fabric/Base/RC/ConstHandle.h>

#if !defined(YYLTYPE) && !defined(YYLTYPE_IS_DECLARED)
typedef struct YYLTYPE
{
  Fabric::RC::ConstHandle<Fabric::RC::String> filename;
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).filename   = YYRHSLOC (Rhs, 1).filename;	\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).filename   = YYRHSLOC (Rhs, 0).filename;	\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif

#include <string>
#include <Fabric/Core/AST/Alias.h>
#include <Fabric/Core/AST/AndOp.h>
#include <Fabric/Core/AST/AssignedVarDecl.h>
#include <Fabric/Core/AST/AssignOp.h>
#include <Fabric/Core/AST/AssignOpImpl.h>
#include <Fabric/Core/AST/BinOp.h>
#include <Fabric/Core/AST/BinOpImpl.h>
#include <Fabric/Core/AST/BreakStatement.h>
#include <Fabric/Core/AST/Call.h>
#include <Fabric/Core/AST/Case.h>
#include <Fabric/Core/AST/CaseVector.h>
#include <Fabric/Core/AST/CompoundStatement.h>
#include <Fabric/Core/AST/ConditionalStatement.h>
#include <Fabric/Core/AST/ConstBoolean.h>
#include <Fabric/Core/AST/ConstDecl.h>
#include <Fabric/Core/AST/ConstDeclStatement.h>
#include <Fabric/Core/AST/ConstUnsignedInteger.h>
#include <Fabric/Core/AST/ConstFloat.h>
#include <Fabric/Core/AST/ConstString.h>
#include <Fabric/Core/AST/ContainerLoop.h>
#include <Fabric/Core/AST/ContinueStatement.h>
#include <Fabric/Core/AST/CreateArrayCache.h>
#include <Fabric/Core/AST/CreateArrayGenerator.h>
#include <Fabric/Core/AST/CreateArrayMap.h>
#include <Fabric/Core/AST/CreateArrayTransform.h>
#include <Fabric/Core/AST/CreateConstArray.h>
#include <Fabric/Core/AST/CreateConstValue.h>
#include <Fabric/Core/AST/CreateReduce.h>
#include <Fabric/Core/AST/CreateValueCache.h>
#include <Fabric/Core/AST/CreateValueGenerator.h>
#include <Fabric/Core/AST/CreateValueTransform.h>
#include <Fabric/Core/AST/CreateValueMap.h>
#include <Fabric/Core/AST/CStyleLoop.h>
#include <Fabric/Core/AST/Destructor.h>
#include <Fabric/Core/AST/ExprStatement.h>
#include <Fabric/Core/AST/ExprVector.h>
#include <Fabric/Core/AST/Function.h>
#include <Fabric/Core/AST/GlobalConstDecl.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/AST/IndexOp.h>
#include <Fabric/Core/AST/InitializedVarDecl.h>
#include <Fabric/Core/AST/MemberDecl.h>
#include <Fabric/Core/AST/MemberDeclVector.h>
#include <Fabric/Core/AST/MethodOp.h>
#include <Fabric/Core/AST/MethodOpImpl.h>
#include <Fabric/Core/AST/NakedVarDecl.h>
#include <Fabric/Core/AST/NotOp.h>
#include <Fabric/Core/AST/Operator.h>
#include <Fabric/Core/AST/OrOp.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/AST/Report.h>
#include <Fabric/Core/AST/Require.h>
#include <Fabric/Core/AST/RequireGlobal.h>
#include <Fabric/Core/AST/RequireVector.h>
#include <Fabric/Core/AST/ReturnStatement.h>
#include <Fabric/Core/AST/StatementVector.h>
#include <Fabric/Core/AST/StructDecl.h>
#include <Fabric/Core/AST/StructMemberOp.h>
#include <Fabric/Core/AST/SwitchStatement.h>
#include <Fabric/Core/AST/TernaryOp.h>
#include <Fabric/Core/AST/Var.h>
#include <Fabric/Core/AST/VarDecl.h>
#include <Fabric/Core/AST/VarDeclStatement.h>
#include <Fabric/Core/AST/VarDeclVector.h>
#include <Fabric/Core/AST/UniOp.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/Util/Parse.h>
using namespace Fabric;

}

%code provides {
#ifndef _FABRIC_KL_PARSER_DECLARED
#define _FABRIC_KL_PARSER_DECLARED
namespace Fabric
{
  namespace CG
  {
    class Diagnostics;
  };
  
  namespace KL
  {
    class Scanner;
    
    RC::ConstHandle<AST::GlobalList> Parse( RC::Handle<Scanner> const &scanner, CG::Diagnostics &diagnostics );
  };
};
#endif //_FABRIC_KL_PARSER_DECLARED
}

%{
#include "Parser.hpp"
#include <Fabric/Core/KL/Context.h>
#include <Fabric/Core/KL/Location.h>
#include <Fabric/Core/KL/Source.h>

#if defined(FABRIC_OS_WINDOWS)
#pragma warning( disable: 4065 )
#endif

void kl_error( YYLTYPE *yyl, KL::Context &context, const char *msg );
int kl_lex( YYSTYPE *yys, YYLTYPE *yyl, KL::Context &context );
%}

%union { std::string const *valueStringPtr; }
%destructor { delete $$; } <valueStringPtr>

%union { CG::UniOpType uniOpType; }
%destructor { } <uniOpType>

%union { CG::BinOpType binOpType; }
%destructor { } <binOpType>

%union { CG::AssignOpType assignOpType; }
%destructor { } <assignOpType>

%union { CG::Usage usage; }
%destructor { } <usage>

%union { Fabric::AST::VarDecl const *astVarDeclPtr; }
%union { Fabric::AST::VarDeclVector const *astVarDeclVectorPtr; }
%union { Fabric::AST::Param const *astParamPtr; }
%union { Fabric::AST::ParamVector const *astParamListPtr; }
%union { Fabric::AST::Global const *astGlobalPtr; }
%union { Fabric::AST::RequireVector const *astRequireVectorPtr; }
%union { Fabric::AST::Require const *astRequirePtr; }
%union { Fabric::AST::GlobalList const *astGlobalListPtr; }
%union { Fabric::AST::StructDecl const *astStructDecl; }
%union { Fabric::AST::MemberDecl const *astStructDeclMember; }
%union { Fabric::AST::MemberDeclVector const *astStructDeclMemberList; }
%union { Fabric::AST::Statement const *astStatementPtr; }
%union { Fabric::AST::StatementVector const *astStatementListPtr; }
%union { Fabric::AST::CompoundStatement const *astCompoundStatementPtr; }
%union { Fabric::AST::ExprVector const *astArgListPtr; }
%union { Fabric::AST::Expr const *astExprPtr; }
%union { Fabric::AST::ConstDecl const *astConstDeclPtr; }
%union { Fabric::AST::Case const *astCasePtr; }
%union { Fabric::AST::CaseVector const *astCaseListPtr; }
%destructor { if ( $$ ) $$->release(); } <*>

%token TOKEN_END 0 "end of file"

%token <valueStringPtr> TOKEN_WHITESPACE "whitespace"

%token <valueStringPtr> TOKEN_COMMENT_BLOCK "block comment"
%token <valueStringPtr> TOKEN_COMMENT_LINE "line comment"

%token TOKEN_IF "if"
%token TOKEN_IN "in"
%token TOKEN_IO "io"
%token TOKEN_DO "do"
%token TOKEN_FOR "for"
%token TOKEN_USE "use"
%token TOKEN_VAR "var"
%token TOKEN_CASE "case"
%token TOKEN_ELSE "else"
%token TOKEN_TRUE "true"
%token TOKEN_ALIAS "alias"
%token TOKEN_BREAK "break"
%token TOKEN_CONST "const"
%token TOKEN_FALSE "false"
%token TOKEN_WHILE "while"
%token TOKEN_REPORT "report"
%token TOKEN_RETURN "return"
%token TOKEN_STRUCT "struct"
%token TOKEN_SWITCH "switch"
%token TOKEN_DEFAULT "default"
%token TOKEN_REQUIRE "require"
%token TOKEN_CONTINUE "continue"
%token TOKEN_FUNCTION "function"
%token TOKEN_OPERATOR "operator"
%token TOKEN_CREATE_REDUCE "createReduce"
%token TOKEN_ARRAY_PRODUCER "ArrayProducer"
%token TOKEN_VALUE_PRODUCER "ValueProducer"
%token TOKEN_CREATE_CONST_ARRAY "createConstArray"
%token TOKEN_CREATE_CONST_VALUE "createConstValue"
%token TOKEN_CREATE_ARRAY_MAP "createArrayMap"
%token TOKEN_CREATE_VALUE_MAP "createValueMap"
%token TOKEN_CREATE_ARRAY_GENERATOR "createArrayGenerator"
%token TOKEN_CREATE_ARRAY_TRANSFORM "createArrayTransform"
%token TOKEN_CREATE_ARRAY_CACHE "createArrayCache"
%token TOKEN_CREATE_VALUE_GENERATOR "createValueGenerator"
%token TOKEN_CREATE_VALUE_TRANSFORM "createValueTransform"
%token TOKEN_CREATE_VALUE_CACHE "createValueCache"

%token TOKEN_LBRACE "{"
%token TOKEN_RBRACE "}"
%token TOKEN_LPAREN "("
%token TOKEN_RPAREN ")"
%token TOKEN_LBRACKET "["
%token TOKEN_RBRACKET "]"
%token TOKEN_COMMA ","
%token TOKEN_SEMICOLON ";"
%token TOKEN_DOT "."
%token TOKEN_COLON ":"
%token TOKEN_QUESTION_MARK "?"

%token TOKEN_PLUS "+"
%token TOKEN_PLUS_PLUS "++"
%token TOKEN_MINUS "-"
%token TOKEN_MINUS_MINUS "--"

%token TOKEN_AST "*"
%token TOKEN_FSLASH "/"
%token TOKEN_PERCENT "%"
%token TOKEN_LANGLE_LANGLE "<<"
%token TOKEN_RANGLE_RANGLE ">>"

%token TOKEN_EXCL "!"
%token TOKEN_AMP_AMP "&&"
%token TOKEN_PIPE_PIPE "||"
%token TOKEN_CARET_CARET "^^"

%token TOKEN_TILDE "~"
%token TOKEN_AMP "&"
%token TOKEN_PIPE "|"
%token TOKEN_CARET "^"

%token TOKEN_EQUALS "="
%token TOKEN_PLUS_EQUALS "+="
%token TOKEN_MINUS_EQUALS "-="
%token TOKEN_AST_EQUALS "*="
%token TOKEN_FSLASH_EQUALS "/="
%token TOKEN_PERCENT_EQUALS "%="
%token TOKEN_AMP_AMP_EQUALS "&&="
%token TOKEN_PIPE_PIPE_EQUALS "||="
%token TOKEN_CARET_CARET_EQUALS "^^="
%token TOKEN_AMP_EQUALS "&="
%token TOKEN_PIPE_EQUALS "|="
%token TOKEN_CARET_EQUALS "^="
%token TOKEN_LANGLE_LANGLE_EQUALS "<<="
%token TOKEN_RANGLE_RANGLE_EQUALS ">>="

%token TOKEN_EQUALS_EQUALS "=="
%token TOKEN_EXCL_EQUALS "!="
%token TOKEN_LANGLE "<"
%token TOKEN_LANGLE_EQUALS "<="
%token TOKEN_RANGLE ">"
%token TOKEN_RANGLE_EQUALS ">="

%token <valueStringPtr> TOKEN_CONST_UI "integer constant"
%token <valueStringPtr> TOKEN_CONST_FP "floating point constant"
%token <valueStringPtr> TOKEN_CONST_STRING_SQUOT "single-quoted string constant"
%token <valueStringPtr> TOKEN_CONST_STRING_DQUOT "double-quoted string constant"

%token <valueStringPtr> TOKEN_IDENTIFIER "identifier"

%type <valueStringPtr> symbol_name
%type <astParamPtr> parameter
%type <astParamPtr> in_parameter
%type <astParamPtr> io_parameter
%type <usage> in_parameter_usage
%type <usage> io_parameter_usage
%type <astParamListPtr> parameter_list
%type <astGlobalPtr> global
%type <astGlobalPtr> function
%type <astGlobalPtr> destructor
%type <astGlobalPtr> prototype
%type <astGlobalPtr> destructor_prototype
%type <astGlobalPtr> alias
%type <astGlobalPtr> struct
%type <astGlobalPtr> global_const_decl
%type <astGlobalPtr> require_global
%type <astRequireVectorPtr> require_list
%type <astRequirePtr> require
%type <astGlobalListPtr> global_list
%type <astStructDeclMember> struct_member
%type <astStructDeclMemberList> struct_member_list
%type <astConstDeclPtr> const_decl
%type <astStatementPtr> const_decl_statement
%type <astStatementPtr> statement
%type <astStatementPtr> expression_statement
%type <valueStringPtr> array_modifier
%type <astStatementPtr> var_decl_statement
%type <astVarDeclPtr> var_decl
%type <astVarDeclVectorPtr> var_decl_list
%type <astStatementPtr> loop_statement
%type <astStatementPtr> switch_statement
%type <astCasePtr> case
%type <astCaseListPtr> case_list
%type <astStatementListPtr> statement_list
%type <astCompoundStatementPtr> compound_statement
%type <astArgListPtr> argument_expression_list
%type <valueStringPtr> simple_type "simple type"
%type <valueStringPtr> compound_type "complex type"

%type <uniOpType> prefix_unary_operator
%type <uniOpType> postfix_unary_operator
%type <binOpType> binary_operator
%type <binOpType> additive_operator
%type <binOpType> shift_operator
%type <binOpType> multiplicative_operator
%type <binOpType> equality_operator
%type <binOpType> relational_operator
%type <assignOpType> assignment_operator

//%type <astExprPtr> constant_expression
%type <astExprPtr> optional_expression
%type <astExprPtr> expression
%type <astExprPtr> assignment_expression
%type <astExprPtr> conditional_expression
%type <astExprPtr> logical_or_expression
%type <astExprPtr> logical_and_expression
%type <astExprPtr> inclusive_or_expression
%type <astExprPtr> exclusive_or_expression
%type <astExprPtr> and_expression
%type <astExprPtr> equality_expression
%type <astExprPtr> relational_expression
%type <astExprPtr> shift_expression
%type <astExprPtr> additive_expression
%type <astExprPtr> multiplicative_expression
%type <astExprPtr> cast_expression
%type <astExprPtr> prefix_unary_expression
%type <astExprPtr> postfix_expression
%type <astExprPtr> primary_expression

%%

start
  : global_list
  {
    context.m_resultGlobalList = $1;
    $1->release();
  }
;

global_list :
  global global_list
  {
    $$ = AST::GlobalList::Create( RC::ConstHandle<AST::Global>($1), $2 ).take();
    $1->release();
    $2->release();
  }
  | /* empty */
  {
    $$ = AST::GlobalList::Create().take();
  }

binary_operator
  : TOKEN_PLUS
  {
    $$ = CG::BIN_OP_ADD;
  }
  | TOKEN_MINUS
  {
    $$ = CG::BIN_OP_SUB;
  }
  | TOKEN_AST
  {
    $$ = CG::BIN_OP_MUL;
  }
  | TOKEN_FSLASH
  {
    $$ = CG::BIN_OP_DIV;
  }
  | TOKEN_PERCENT
  {
    $$ = CG::BIN_OP_MOD;
  }
  | TOKEN_LANGLE_LANGLE
  {
    $$ = CG::BIN_OP_SHL;
  }
  | TOKEN_RANGLE_RANGLE
  {
    $$ = CG::BIN_OP_SHR;
  }
;

global_const_decl
  : const_decl
  {
    $$ = AST::GlobalConstDecl::Create( RTLOC, $1 ).take();
    $1->release();
  }
;

global
  : function
  {
    $$ = $1;
  }
  | prototype
  {
    $$ = $1;
  }
  | alias
  {
    $$ = $1;
  }
  | struct
  {
    $$ = $1;
  }
  | global_const_decl
  {
    $$ = $1;
  }
  | require_global
  {
    $$ = $1;
  }
;

require_global
  : TOKEN_REQUIRE require_list TOKEN_SEMICOLON
  {
    $$ = AST::RequireGlobal::Create( RTLOC, $2 ).take();
    $2->release();
  }
  | TOKEN_USE require_list TOKEN_SEMICOLON
  {
    $$ = AST::RequireGlobal::Create( RTLOC, $2 ).take();
    $2->release();
  }
;

require_list
  : /* empty */
  {
    $$ = AST::RequireVector::Create().take();
  }
  | require
  {
    $$ = AST::RequireVector::Create( $1 ).take();
    $1->release();
  }
  | require TOKEN_COMMA require_list
  {
    $$ = AST::RequireVector::Create( $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

require
  : TOKEN_IDENTIFIER
  {
    $$ = AST::Require::Create( RTLOC, *$1 ).take();
    delete $1;
  }
;

function
  : TOKEN_FUNCTION compound_type TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::Function::Create( RTLOC, *$2, *$3, $5, $7, $8 ).take();
    delete $2;
    delete $3;
    $5->release();
    delete $7;
    $8->release();
  }
  | TOKEN_FUNCTION TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::Function::Create( RTLOC, "", *$2, $4, $6, $7 ).take();
    delete $2;
    $4->release();
    delete $6;
    $7->release();
  }
  | TOKEN_OPERATOR TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::Operator::Create( RTLOC, *$2, $4, $6, $7 ).take();
    delete $2;
    $4->release();
    delete $6;
    $7->release();
  }
  | TOKEN_FUNCTION compound_type TOKEN_IDENTIFIER TOKEN_DOT TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::MethodOpImpl::Create( RTLOC, *$2, *$3, *$5, $7, $9, $10 ).take();
    delete $2;
    delete $3;
    delete $5;
    $7->release();
    delete $9;
    $10->release();
  }
  | TOKEN_FUNCTION compound_type TOKEN_DOT TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::MethodOpImpl::Create( RTLOC, "", *$2, *$4, $6, $8, $9 ).take();
    delete $2;
    delete $4;
    $6->release();
    delete $8;
    $9->release();
  }
  | TOKEN_FUNCTION compound_type binary_operator TOKEN_LPAREN in_parameter TOKEN_COMMA in_parameter TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::BinOpImpl::Create( RTLOC, *$2, $3, $5, $7, $9, $10 ).take();
    delete $2;
    $5->release();
    $7->release();
    delete $9;
    $10->release();
  }
  | TOKEN_FUNCTION compound_type TOKEN_DOT assignment_operator TOKEN_LPAREN in_parameter TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::AssignOpImpl::Create( RTLOC, *$2, $4, $6, $8, $9 ).take();
    delete $2;
    $6->release();
    delete $8;
    $9->release();
  }
  | destructor
;

destructor
  : TOKEN_FUNCTION TOKEN_TILDE compound_type TOKEN_LPAREN TOKEN_RPAREN symbol_name compound_statement
  {
    $$ = AST::Destructor::Create( RTLOC, *$3, $6, $7 ).take();
    delete $3;
    delete $6;
    $7->release();
  }
;

prototype
  : TOKEN_FUNCTION compound_type TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name TOKEN_SEMICOLON
  {
    $$ = AST::Function::Create( RTLOC, *$2, *$3, $5, $7, 0 ).take();
    delete $2;
    delete $3;
    $5->release();
    delete $7;
  }
  | TOKEN_FUNCTION TOKEN_IDENTIFIER TOKEN_LPAREN parameter_list TOKEN_RPAREN symbol_name TOKEN_SEMICOLON
  {
    $$ = AST::Function::Create( RTLOC, "", *$2, $4, $6, 0 ).take();
    delete $2;
    $4->release();
    delete $6;
  }
  | destructor_prototype
;

destructor_prototype
  : TOKEN_FUNCTION TOKEN_TILDE compound_type TOKEN_LPAREN TOKEN_RPAREN symbol_name TOKEN_SEMICOLON
  {
    $$ = AST::Destructor::Create( RTLOC, *$3, $6, 0 ).take();
    delete $3;
    delete $6;
  }
;

symbol_name
  : TOKEN_EQUALS TOKEN_CONST_STRING_SQUOT
  {
    $$ = new std::string( Util::parseQuotedString( *$2 ) );
    delete $2;
  }
  | TOKEN_EQUALS TOKEN_CONST_STRING_DQUOT
  {
    $$ = new std::string( Util::parseQuotedString( *$2 ) );
    delete $2;
  }
  | /* empty */
  {
    $$ = 0;
  }
;

alias
  : TOKEN_ALIAS TOKEN_IDENTIFIER TOKEN_IDENTIFIER array_modifier TOKEN_SEMICOLON
  {
    std::string compoundTypeName( *$2 );
    delete $2;
    compoundTypeName.append( *$4 );
    delete $4;
    
    $$ = AST::Alias::Create( RTLOC, *$3, compoundTypeName ).take();
    delete $3;
  }
;

struct
  : TOKEN_STRUCT TOKEN_IDENTIFIER TOKEN_LBRACE struct_member_list TOKEN_RBRACE TOKEN_SEMICOLON
  {
    $$ = AST::StructDecl::Create( RTLOC, *$2, $4 ).take();
    delete $2;
    $4->release();
  }
;

struct_member_list
  : struct_member struct_member_list
  {
    $$ = AST::MemberDeclVector::Create( $1, $2 ).take();
    $1->release();
    $2->release();
  }
  | /* empty */
  {
    $$ = AST::MemberDeclVector::Create().take();
  }
;

struct_member
  : TOKEN_IDENTIFIER TOKEN_IDENTIFIER array_modifier TOKEN_SEMICOLON
  {
    std::string compoundTypeName( *$1 );
    delete $1;
    compoundTypeName.append( *$3 );
    delete $3;
    
    $$ = AST::MemberDecl::Create( RTLOC, *$2, compoundTypeName ).take();
    delete $2;
  }
;

parameter_list
  : /* empty */
  {
    $$ = AST::ParamVector::Create().take();
  }
  | parameter
  {
    $$ = AST::ParamVector::Create( $1 ).take();
    $1->release();
  }
  | parameter TOKEN_COMMA parameter_list
  {
    $$ = AST::ParamVector::Create( $1, RC::ConstHandle<AST::ParamVector>( $3 ) ).take();
    $1->release();
    $3->release();
  }
;

const_decl
  : TOKEN_CONST TOKEN_IDENTIFIER TOKEN_IDENTIFIER TOKEN_EQUALS TOKEN_CONST_UI TOKEN_SEMICOLON
  {
    $$ = AST::ConstDecl::Create( RTLOC, *$3, *$2, *$5 ).take();
    delete $2;
    delete $3;
    delete $5;
  }
  | TOKEN_CONST TOKEN_IDENTIFIER TOKEN_IDENTIFIER TOKEN_EQUALS TOKEN_CONST_FP TOKEN_SEMICOLON
  {
    $$ = AST::ConstDecl::Create( RTLOC, *$3, *$2, *$5 ).take();
    delete $2;
    delete $3;
    delete $5;
  }
;

const_decl_statement
  : const_decl
  {
    $$ = AST::ConstDeclStatement::Create( RTLOC, $1 ).take();
    $1->release();
  }
;

array_modifier
  : TOKEN_LANGLE TOKEN_RANGLE array_modifier
  {
    std::string *result = new std::string( "<>" );
    result->append( *$3 );
    delete $3;
    
    $$ = result;
  }
  | TOKEN_LBRACKET TOKEN_RBRACKET array_modifier
  {
    std::string *result = new std::string( "[]" );
    result->append( *$3 );
    delete $3;
    
    $$ = result;
  }
  | TOKEN_LBRACKET TOKEN_CONST_UI TOKEN_RBRACKET array_modifier
  {
    uint64_t length = Util::parseUInt64( *$2 );
    delete $2;

    if ( length == 0 )
    {
      delete $4;
      yyerror( &yyloc, context, "fixed array size must be greater than zero" );
      YYERROR;
    }
    else if ( length > SIZE_MAX )
    {
      delete $4;
      yyerror( &yyloc, context, "fixed array size too large" );
      YYERROR;
    }
    else
    {
      std::string *result = new std::string( "[" + _(length) + "]" );
      result->append( *$4 );
      delete $4;
      
      $$ = result;
    }
  }
  | TOKEN_LBRACKET compound_type TOKEN_RBRACKET array_modifier
  {
    std::string *result = new std::string( "[" + *$2 + "]" );
    delete $2;
    result->append( *$4 );
    delete $4;
    $$ = result;
  }
  | /* empty */
  {
    $$ = new std::string;
  }
;

var_decl
  : TOKEN_IDENTIFIER array_modifier TOKEN_EQUALS assignment_expression
  {
    $$ = AST::AssignedVarDecl::Create( RTLOC, *$1, *$2, $4 ).take();
    delete $1;
    delete $2;
    $4->release();
  }
  | TOKEN_IDENTIFIER array_modifier TOKEN_LPAREN argument_expression_list TOKEN_RPAREN
  {
    $$ = AST::InitializedVarDecl::Create( RTLOC, *$1, *$2, $4 ).take();
    delete $1;
    delete $2;
    $4->release();
  }
  | TOKEN_IDENTIFIER array_modifier 
  {
    $$ = AST::NakedVarDecl::Create( RTLOC, *$1, *$2 ).take();
    delete $1;
    delete $2;
  }
;

var_decl_list
  : var_decl
  {
    $$ = AST::VarDeclVector::Create( $1 ).take();
    $1->release();
  }
  | var_decl TOKEN_COMMA var_decl_list
  {
    $$ = AST::VarDeclVector::Create( $1, $3 ).take();
    $1->release();
    $3->release();
  }
  ;

in_parameter_usage
  : TOKEN_IN
  {
    $$ = CG::USAGE_RVALUE;
  }
  | /* empty */
  {
    $$ = CG::USAGE_RVALUE;
  }
;

io_parameter_usage
  : TOKEN_IO
  {
    $$ = CG::USAGE_LVALUE;
  }
;

in_parameter
  : in_parameter_usage simple_type TOKEN_IDENTIFIER array_modifier
  {
    std::string compoundTypeName( *$2 );
    delete $2;
    compoundTypeName.append( *$4 );
    delete $4;
    
    $$ = AST::Param::Create( RTLOC, *$3, compoundTypeName, $1 ).take();
    delete $3;
  }
;

io_parameter
  : io_parameter_usage simple_type TOKEN_IDENTIFIER array_modifier
  {
    std::string compoundTypeName( *$2 );
    delete $2;
    compoundTypeName.append( *$4 );
    delete $4;
    
    $$ = AST::Param::Create( RTLOC, *$3, compoundTypeName, $1 ).take();
    delete $3;
  }
;

parameter
  : in_parameter
  {
    $$ = $1;
  }
  | io_parameter
  {
    $$ = $1;
  }
;

compound_statement
  : TOKEN_LBRACE statement_list TOKEN_RBRACE
  {
    $$ = AST::CompoundStatement::Create( RTLOC, $2 ).take();
    $2->release();
  }
;

statement_list
  : statement statement_list
  {
    if ( $1 )
    {
      $$ = AST::StatementVector::Create( $1, $2 ).take();
      $1->release();
      $2->release();
    }
    else $$ = $2;
  }
  | /* empty */
  {
    $$ = AST::StatementVector::Create().take();
  }
;

simple_type
  : TOKEN_IDENTIFIER
  {
    $$ = $1;
  }
  | TOKEN_VALUE_PRODUCER TOKEN_LANGLE compound_type TOKEN_RANGLE
  {
    std::string *result = new std::string( "ValueProducer<" );
    result->append( *$3 );
    delete $3;
    result->append( ">" );
    $$ = result;
  }
  | TOKEN_ARRAY_PRODUCER TOKEN_LANGLE compound_type TOKEN_RANGLE
  {
    std::string *result = new std::string( "ArrayProducer<" );
    result->append( *$3 );
    delete $3;
    result->append( ">" );
    $$ = result;
  }
;

compound_type
  : simple_type array_modifier
  {
    std::string *result = new std::string( *$1 );
    delete $1;
    result->append( *$2 );
    delete $2;
    
    $$ = result;
  }
;

var_decl_statement
  : simple_type var_decl_list TOKEN_SEMICOLON 
  {
    $$ = AST::VarDeclStatement::Create( RTLOC, *$1, $2 ).take();
    delete $1;
    $2->release();
  }
  | TOKEN_VAR compound_type var_decl_list TOKEN_SEMICOLON 
  {
    $$ = AST::VarDeclStatement::Create( RTLOC, *$2, $3 ).take();
    delete $2;
    $3->release();
  }
;

loop_statement
  : TOKEN_FOR TOKEN_LPAREN statement optional_expression TOKEN_SEMICOLON optional_expression TOKEN_RPAREN statement
  {
    $$ = AST::CStyleLoop::Create( RTLOC, $3, $4, $6, 0, $8 ).take();
    if ( $3 )
      $3->release();
    if ( $4 )
      $4->release();
    if ( $6 )
      $6->release();
    if ( $8 )
      $8->release();
  }
  | TOKEN_FOR TOKEN_LPAREN TOKEN_IDENTIFIER TOKEN_IN expression TOKEN_RPAREN statement
  {
    $$ = AST::ContainerLoop::Create( RTLOC, $5, *$3, "", $7 ).take();
    $5->release();
    delete $3;
    $7->release();
  }
  | TOKEN_FOR TOKEN_LPAREN TOKEN_IDENTIFIER TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_IN expression TOKEN_RPAREN statement
  {
    $$ = AST::ContainerLoop::Create( RTLOC, $7, *$3, *$5, $9 ).take();
    $7->release();
    delete $3;
    delete $5;
    $9->release();
  }
  | TOKEN_WHILE TOKEN_LPAREN optional_expression TOKEN_RPAREN statement
  {
    $$ = AST::CStyleLoop::Create( RTLOC, 0, $3, 0, 0, $5 ).take();
    if ( $3 )
      $3->release();
    $5->release();
  }
  | TOKEN_DO statement TOKEN_WHILE TOKEN_LPAREN optional_expression TOKEN_RPAREN TOKEN_SEMICOLON
  {
    $$ = AST::CStyleLoop::Create( RTLOC, 0, 0, 0, $5, $2 ).take();
    $2->release();
    if ( $5 )
      $5->release();
  }
;

switch_statement
  : TOKEN_SWITCH TOKEN_LPAREN expression TOKEN_RPAREN TOKEN_LBRACE case_list TOKEN_RBRACE
  {
    $$ = AST::SwitchStatement::Create( RTLOC, $3, $6 ).take();
    $3->release();
    $6->release();
  }
;

case_list
  : case case_list
  {
    $$ = AST::CaseVector::Create( $1, $2 ).take();
    $1->release();
    $2->release();
  }
  | /* empty */
  {
    $$ = AST::CaseVector::Create().take();
  }
;

case
  : TOKEN_CASE expression TOKEN_COLON statement_list
  {
    $$ = AST::Case::Create( RTLOC, $2, $4 ).take();
    $2->release();
    $4->release();
  }
  | TOKEN_DEFAULT TOKEN_COLON statement_list
  {
    $$ = AST::Case::Create( RTLOC, 0, $3 ).take();
    $3->release();
  }
;

statement
  : TOKEN_SEMICOLON
  {
    $$ = 0;
  }
  | TOKEN_RETURN expression TOKEN_SEMICOLON
  {
    $$ = AST::ReturnStatement::Create( RTLOC, $2 ).take();
    $2->release();
  }
  | TOKEN_RETURN TOKEN_SEMICOLON
  {
    $$ = AST::ReturnStatement::Create( RTLOC ).take();
  }
  | const_decl_statement
  | var_decl_statement
  | expression_statement
  | TOKEN_IF TOKEN_LPAREN expression TOKEN_RPAREN statement TOKEN_ELSE statement
  {
    $$ = AST::ConditionalStatement::Create( RTLOC, $3, $5, $7 ).take();
    $3->release();
    $5->release();
    $7->release();
  }
  | TOKEN_IF TOKEN_LPAREN expression TOKEN_RPAREN statement
  {
    $$ = AST::ConditionalStatement::Create( RTLOC, $3, $5 ).take();
    $3->release();
    $5->release();
  }
  | loop_statement
  | TOKEN_REPORT expression TOKEN_SEMICOLON
  {
    $$ = AST::Report::Create( RTLOC, $2 ).take();
    $2->release();
  }
  | TOKEN_BREAK TOKEN_SEMICOLON
  {
    $$ = AST::BreakStatement::Create( RTLOC ).take();
  }
  | TOKEN_CONTINUE TOKEN_SEMICOLON
  {
    $$ = AST::ContinueStatement::Create( RTLOC ).take();
  }
  | switch_statement
  | compound_statement
  {
    $$ = $1;
  }
;

expression_statement
  : expression TOKEN_SEMICOLON
  {
    $$ = AST::ExprStatement::Create( RTLOC, $1 ).take();
    $1->release();
  }
;

/*
constant_expression
	: conditional_expression
  {
    $$ = $1;
  }
;
*/

optional_expression
  : expression
  | /* empty */
  {
    $$ = 0;
  }
;

expression
	: assignment_expression
  {
    $$ = $1;
  }
  /*
	| expression ',' assignment_expression
  */
;

assignment_operator
	: TOKEN_EQUALS
  {
    $$ = CG::ASSIGN_OP;
  }
  | TOKEN_PLUS_EQUALS
  {
    $$ = CG::ASSIGN_OP_ADD;
  }
  | TOKEN_MINUS_EQUALS
  {
    $$ = CG::ASSIGN_OP_SUB;
  }
  | TOKEN_AST_EQUALS
  {
    $$ = CG::ASSIGN_OP_MUL;
  }
  | TOKEN_FSLASH_EQUALS
  {
    $$ = CG::ASSIGN_OP_DIV;
  }
  | TOKEN_PERCENT_EQUALS
  {
    $$ = CG::ASSIGN_OP_MOD;
  }
  | TOKEN_PIPE_EQUALS
  {
    $$ = CG::ASSIGN_OP_BIT_OR;
  }
  | TOKEN_AMP_EQUALS
  {
    $$ = CG::ASSIGN_OP_BIT_AND;
  }
  | TOKEN_CARET_EQUALS
  {
    $$ = CG::ASSIGN_OP_BIT_XOR;
  }
  | TOKEN_LANGLE_LANGLE_EQUALS
  {
    $$ = CG::ASSIGN_OP_SHL;
  }
  | TOKEN_RANGLE_RANGLE_EQUALS
  {
    $$ = CG::ASSIGN_OP_SHR;
  }
;

assignment_expression
	: conditional_expression
  {
    $$ = $1;
  }
	| postfix_expression assignment_operator assignment_expression
  {
    $$ = AST::AssignOp::Create( RTLOC, $2, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

conditional_expression
	: logical_or_expression
  {
    $$ = $1;
  }
	| logical_or_expression TOKEN_QUESTION_MARK expression TOKEN_COLON conditional_expression
  {
    $$ = AST::TernaryOp::Create( RTLOC, CG::TERNARY_OP_COND, $1, $3, $5 ).take();
    $1->release();
    $3->release();
    $5->release();
  }
;

logical_or_expression
	: logical_and_expression
  {
    $$ = $1;
  }
	| logical_or_expression TOKEN_PIPE_PIPE logical_and_expression
  {
    $$ = AST::OrOp::Create( RTLOC, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

logical_and_expression
	: inclusive_or_expression
  {
    $$ = $1;
  }
	| logical_and_expression TOKEN_AMP_AMP inclusive_or_expression
  {
    $$ = AST::AndOp::Create( RTLOC, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

inclusive_or_expression
	: exclusive_or_expression
  {
    $$ = $1;
  }
	| inclusive_or_expression TOKEN_PIPE exclusive_or_expression
  {
    $$ = AST::BinOp::Create( RTLOC, CG::BIN_OP_BIT_OR, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

exclusive_or_expression
	: and_expression
  {
    $$ = $1;
  }
	| exclusive_or_expression TOKEN_CARET and_expression
  {
    $$ = AST::BinOp::Create( RTLOC, CG::BIN_OP_BIT_XOR, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

and_expression
	: equality_expression
  {
    $$ = $1;
  }
	| and_expression TOKEN_AMP equality_expression
  {
    $$ = AST::BinOp::Create( RTLOC, CG::BIN_OP_BIT_AND, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

equality_operator
  : TOKEN_EQUALS_EQUALS
  {
    $$ = CG::BIN_OP_EQ;
  }
  | TOKEN_EXCL_EQUALS
  {
    $$ = CG::BIN_OP_NE;
  }
;

equality_expression
	: relational_expression
  {
    $$ = $1;
  }
	| equality_expression equality_operator relational_expression
  {
    $$ = AST::BinOp::Create( RTLOC, $2, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

relational_operator
  : TOKEN_LANGLE
  {
    $$ = CG::BIN_OP_LT;
  }
  | TOKEN_LANGLE_EQUALS
  {
    $$ = CG::BIN_OP_LE;
  }
  | TOKEN_RANGLE
  {
    $$ = CG::BIN_OP_GT;
  }
  | TOKEN_RANGLE_EQUALS
  {
    $$ = CG::BIN_OP_GE;
  }
;

relational_expression
	: shift_expression
  {
    $$ = $1;
  }
	| relational_expression relational_operator shift_expression
  {
    $$ = AST::BinOp::Create( RTLOC, $2, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

shift_operator
  : TOKEN_LANGLE_LANGLE
  {
    $$ = CG::BIN_OP_SHL;
  }
  | TOKEN_RANGLE_RANGLE
  {
    $$ = CG::BIN_OP_SHR;
  }
;

shift_expression
	: additive_expression
  {
    $$ = $1;
  }
	| shift_expression shift_operator additive_expression
  {
    $$ = AST::BinOp::Create( RTLOC, $2, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

additive_operator
  : TOKEN_PLUS
  {
    $$ = CG::BIN_OP_ADD;
  }
  | TOKEN_MINUS
  {
    $$ = CG::BIN_OP_SUB;
  }
;

additive_expression
	: multiplicative_expression
  {
    $$ = $1;
  }
	| additive_expression additive_operator multiplicative_expression
  {
    $$ = AST::BinOp::Create( RTLOC, $2, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

multiplicative_operator
  : TOKEN_AST
  {
    $$ = CG::BIN_OP_MUL;
  }
  | TOKEN_FSLASH
  {
    $$ = CG::BIN_OP_DIV;
  }
  | TOKEN_PERCENT
  {
    $$ = CG::BIN_OP_MOD;
  }
;

multiplicative_expression
  : cast_expression
  {
    $$ = $1;
  }
  | multiplicative_expression multiplicative_operator cast_expression
  {
    $$ = AST::BinOp::Create( RTLOC, $2, $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

cast_expression
	: prefix_unary_expression
  {
    $$ = $1;
  }
;

prefix_unary_operator
  : TOKEN_PLUS
  {
    $$ = CG::UNI_OP_POS;
  }
  | TOKEN_MINUS
  {
    $$ = CG::UNI_OP_NEG;
  }
  | TOKEN_TILDE
  {
    $$ = CG::UNI_OP_BIT_NOT;
  }
  | TOKEN_PLUS_PLUS
  {
    $$ = CG::UNI_OP_PRE_INC;
  }
  | TOKEN_MINUS_MINUS
  {
    $$ = CG::UNI_OP_PRE_DEC;
  }
;

prefix_unary_expression
	: postfix_expression
  {
    $$ = $1;
  }
	| prefix_unary_operator cast_expression
  {
    $$ = AST::UniOp::Create( RTLOC, $1, $2 ).take();
    $2->release();
  }
  | TOKEN_EXCL cast_expression
  {
    $$ = AST::NotOp::Create( RTLOC, $2 ).take();
    $2->release();
  }
  /*
	| SIZEOF unary_expression
	| SIZEOF '(' type_name ')'
  */
;

argument_expression_list
	: /* empty */
  {
    $$ = AST::ExprVector::Create().take();
  }
  | assignment_expression
  {
    $$ = AST::ExprVector::Create( $1 ).take();
    $1->release();
  }
	| assignment_expression TOKEN_COMMA argument_expression_list
  {
    $$ = AST::ExprVector::Create( $1, $3 ).take();
    $1->release();
    $3->release();
  }
;

postfix_unary_operator
  : TOKEN_PLUS_PLUS
  {
    $$ = CG::UNI_OP_POST_INC;
  }
  | TOKEN_MINUS_MINUS
  {
    $$ = CG::UNI_OP_POST_DEC;
  }
;

postfix_expression
	: primary_expression
  {
    $$ = $1;
  }
  | TOKEN_IDENTIFIER TOKEN_LPAREN argument_expression_list TOKEN_RPAREN
  {
    $$ = AST::Call::Create( RTLOC, *$1, $3 ).take();
    delete $1;
    $3->release();
  }
  | TOKEN_CREATE_CONST_VALUE TOKEN_LPAREN assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateConstValue::Create( RTLOC, $3 ).take();
    $3->release();
  }
  | TOKEN_CREATE_VALUE_GENERATOR TOKEN_LPAREN TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateValueGenerator::Create( RTLOC, *$3, 0 ).take();
    delete $3;
  }
  | TOKEN_CREATE_VALUE_GENERATOR TOKEN_LPAREN TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateValueGenerator::Create( RTLOC, *$3, $5 ).take();
    delete $3;
    $5->release();
  }
  | TOKEN_CREATE_VALUE_TRANSFORM TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateValueTransform::Create( RTLOC, $3, *$5, 0 ).take();
    $3->release();
    delete $5;
  }
  | TOKEN_CREATE_VALUE_TRANSFORM TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateValueTransform::Create( RTLOC, $3, *$5, $7 ).take();
    $3->release();
    delete $5;
    $7->release();
  }
  | TOKEN_CREATE_VALUE_MAP TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateValueMap::Create( RTLOC, $3, *$5, 0 ).take();
    $3->release();
    delete $5;
  }
  | TOKEN_CREATE_VALUE_MAP TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateValueMap::Create( RTLOC, $3, *$5, $7 ).take();
    $3->release();
    delete $5;
    $7->release();
  }
  | TOKEN_CREATE_VALUE_CACHE TOKEN_LPAREN assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateValueCache::Create( RTLOC, $3 ).take();
    $3->release();
  }
  | TOKEN_CREATE_CONST_ARRAY TOKEN_LPAREN assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateConstArray::Create( RTLOC, $3 ).take();
    $3->release();
  }
  | TOKEN_CREATE_ARRAY_GENERATOR TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateArrayGenerator::Create( RTLOC, $3, *$5, 0 ).take();
    $3->release();
    delete $5;
  }
  | TOKEN_CREATE_ARRAY_GENERATOR TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateArrayGenerator::Create( RTLOC, $3, *$5, $7 ).take();
    $3->release();
    delete $5;
    $7->release();
  }
  | TOKEN_CREATE_ARRAY_MAP TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateArrayMap::Create( RTLOC, $3, *$5, 0 ).take();
    $3->release();
    delete $5;
  }
  | TOKEN_CREATE_ARRAY_MAP TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateArrayMap::Create( RTLOC, $3, *$5, $7 ).take();
    $3->release();
    delete $5;
    $7->release();
  }
  | TOKEN_CREATE_ARRAY_TRANSFORM TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateArrayTransform::Create( RTLOC, $3, *$5, 0 ).take();
    $3->release();
    delete $5;
  }
  | TOKEN_CREATE_ARRAY_TRANSFORM TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateArrayTransform::Create( RTLOC, $3, *$5, $7 ).take();
    $3->release();
    delete $5;
    $7->release();
  }
  | TOKEN_CREATE_ARRAY_CACHE TOKEN_LPAREN assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateArrayCache::Create( RTLOC, $3 ).take();
    $3->release();
  }
  | TOKEN_CREATE_REDUCE TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_RPAREN
  {
    $$ = AST::CreateReduce::Create( RTLOC, $3, *$5, 0 ).take();
    $3->release();
    delete $5;
  }
  | TOKEN_CREATE_REDUCE TOKEN_LPAREN assignment_expression TOKEN_COMMA TOKEN_IDENTIFIER TOKEN_COMMA assignment_expression TOKEN_RPAREN
  {
    $$ = AST::CreateReduce::Create( RTLOC, $3, *$5, $7 ).take();
    $3->release();
    delete $5;
    $7->release();
  }
	| postfix_expression TOKEN_LBRACKET expression TOKEN_RBRACKET
  {
    $$ = AST::IndexOp::Create( RTLOC, $1, $3 ).take();
    $1->release();
    $3->release();
  }
	| postfix_expression TOKEN_DOT TOKEN_IDENTIFIER
  {
    $$ = AST::StructMemberOp::Create( RTLOC, $1, *$3 ).take();
    $1->release();
    delete $3;
  }
  | postfix_expression TOKEN_DOT TOKEN_IDENTIFIER TOKEN_LPAREN argument_expression_list TOKEN_RPAREN
  {
    $$ = AST::MethodOp::Create( RTLOC, *$3, $1, $5 ).take();
    $1->release();
    delete $3;
    $5->release();
  }
  /*
	| postfix_expression PTR_OP IDENTIFIER
  */
	| postfix_expression postfix_unary_operator
  {
    $$ = AST::UniOp::Create( RTLOC, $2, $1 ).take();
    $1->release();
  }
;

primary_expression
  : TOKEN_TRUE
  {
    $$ = AST::ConstBoolean::Create( RTLOC, true ).take();
  }
  | TOKEN_FALSE
  {
    $$ = AST::ConstBoolean::Create( RTLOC, false ).take();
  }
  | TOKEN_CONST_UI
  {
    $$ = AST::ConstUnsignedInteger::Create( RTLOC, *$1 ).take();
    delete $1;
  }
  | TOKEN_CONST_FP
  {
    $$ = AST::ConstFloat::Create( RTLOC, *$1 ).take();
    delete $1;
  }
  | TOKEN_CONST_STRING_SQUOT
  {
    try
    {
      $$ = AST::ConstString::Create( RTLOC, *$1 ).take();
    }
    catch ( Exception e )
    {
      context.m_diagnostics.addError( RTLOC, e.getDesc() );
    }
    delete $1;
  }
  | TOKEN_CONST_STRING_DQUOT
  {
    try
    {
      $$ = AST::ConstString::Create( RTLOC, *$1 ).take();
    }
    catch ( Exception e )
    {
      context.m_diagnostics.addError( RTLOC, e.getDesc() );
    }
    delete $1;
  }
  | TOKEN_IDENTIFIER
  {
    $$ = AST::Var::Create( RTLOC, *$1 ).take();
    delete $1;
  }
  | TOKEN_LPAREN expression TOKEN_RPAREN
  {
    $$ = $2;
  }
;

%%

#include <Fabric/Core/KL/Source.h>

void kl_error( YYLTYPE *yyl, KL::Context &context, const char *msg )
{
  context.m_diagnostics.addError( CG::Location( yyl->filename, yyl->first_line, yyl->first_column ), msg );
}

int kl_lex( YYSTYPE *yys, YYLTYPE *yyl, KL::Context &context )
{
  RC::ConstHandle<KL::Source> source = context.m_scanner->getSourceReader().getSource();
  KL::Location startLocation( source->filename() );
  KL::Token token( source );
  std::string text;
  KL::Location endLocation( source->filename() );
  while ( true )
  {
    startLocation = context.m_scanner->getLocationForStart();
    try
    {
      token = context.m_scanner->nextToken();
    }
    catch ( Exception e )
    {
      context.m_diagnostics.addError( startLocation, e.getDesc() );
    }
    if ( token.getType() != TOKEN_WHITESPACE
      && token.getType() != TOKEN_COMMENT_LINE
      && token.getType() != TOKEN_COMMENT_BLOCK
      )
    {
      endLocation = context.m_scanner->getLocationForEnd();
      break;
    }
  }
  
  if ( token.getType() == TOKEN_CONST_UI
    || token.getType() == TOKEN_CONST_FP
    || token.getType() == TOKEN_CONST_STRING_SQUOT
    || token.getType() == TOKEN_CONST_STRING_DQUOT
    || token.getType() == TOKEN_IDENTIFIER )
    yys->valueStringPtr = new std::string( token.toString() );

  yyl->filename = startLocation.getFilename();
  yyl->first_line = startLocation.getLine();
  yyl->first_column = startLocation.getColumn();
  yyl->last_line = endLocation.getLine();
  yyl->last_column = endLocation.getColumn();
  
  int result = int( token.getType() );
  if ( kl_debug )
    fprintf( stderr, "Returning token %d\n", result );
  return result;
}

namespace Fabric
{
  namespace KL
  {
    RC::ConstHandle<AST::GlobalList> Parse( RC::Handle<Scanner> const &scanner, CG::Diagnostics &diagnostics )
    {
      Context context( scanner, diagnostics );

      // TODO: Do we check the status?
      kl_debug = 0;

      kl_parse( context );

      return context.m_resultGlobalList;
    }
  };
};
