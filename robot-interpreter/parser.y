%code requires {
    #include "ast.hpp"
}

%{
    #include "ast.hpp"

    #include <cstdio>
    #include <cstdlib>
    #include <iostream>

    extern int yylex();
    extern int yylineno;

    void yyerror(const char* message);

    Program* g_program = nullptr;
%}

%union {
    char* str;

    long long int_value;
    unsigned long long uint_value;

    bool boolean;

    ValueType value_type;
    CommandKind command;

    Expression* expression;
    ExpressionList* expression_list;

    Parameter* parameter;
    ParameterList* parameter_list;

    Statement* statement;
    StatementList* statement_list;

    FunctionDecl* function;
    FunctionList* function_list;
}

%token FUNC

%token SIGNED
%token UNSIGNED
%token CONST

%token TESTONCE
%token TESTREP
%token CALL

%token TOP
%token BOTTOM
%token LEFT
%token RIGHT
%token XRAY

%token ASSIGN

%token LESS
%token GREATER
%token EQUAL

%token PLUS
%token MINUS
%token STAR
%token SLASH
%token PERCENT

%token LPAREN
%token RPAREN
%token SEMICOLON
%token COMMA

%token <str> IDENT
%token <int_value> INT_LITERAL
%token <uint_value> UINT_LITERAL

%type <boolean> const_opt
%type <value_type> type
%type <command> robot_command
%type <expression> expression
%type <expression> init_opt
%type <statement> statement
%type <statement_list> statements
%type <statement_list> block
%type <function> function
%type <function_list> functions

%type <parameter> parameter
%type <parameter_list> parameters
%type <parameter_list> parameters_opt

%type <expression_list> arguments
%type <expression_list> arguments_opt

%left PLUS MINUS
%left LESS GREATER EQUAL
%left STAR SLASH PERCENT
%right UMINUS

%%

program:
      functions
      {
          g_program = new Program($1);
      }
    ;

functions:
      function
      {
          $$ = new FunctionList();
          $$->push_back($1);
      }
    | functions function
      {
          $$ = $1;
          $$->push_back($2);
      }
    ;

function:
      FUNC IDENT LPAREN parameters_opt RPAREN block
      {
          $$ = new FunctionDecl($2, $4, $6);
          free($2);
      }
    ;

parameters_opt:
      /* empty */
      {
          $$ = new ParameterList();
      }
    | parameters
      {
          $$ = $1;
      }
    ;

parameters:
      parameter
      {
          $$ = new ParameterList();
          $$->push_back($1);
      }
    | parameters COMMA parameter
      {
          $$ = $1;
          $$->push_back($3);
      }
    ;

parameter:
      type IDENT
      {
          $$ = new Parameter($1, $2);
          free($2);
      }
    ;

block:
      LPAREN statements RPAREN
      {
          $$ = $2;
      }
    ;

statements:
      {
          $$ = new StatementList();
      }
    | statements statement
      {
          $$ = $1;
          $$->push_back($2);
      }
    ;

statement:
      robot_command SEMICOLON
      {
          $$ = new MoveStatement($1);
      }
      | XRAY SEMICOLON
      {
        $$ = new XrayStatement();
      }
        // init
      | const_opt type IDENT init_opt SEMICOLON
      {
        $$ = new VarDeclStatement($2, $3, $1, $4);
        free($3);
      }
        // assig
      | IDENT ASSIGN expression SEMICOLON
      {
        $$ = new AssignmentStatement($1, $3);
        free($1);
      }
      | TESTONCE LPAREN expression RPAREN block
      {
        $$ = new TestOnceStatement($3, $5);
      }
      | TESTREP LPAREN expression RPAREN block
      {
        $$ = new TestRepStatement($3, $5);
      }
      | CALL IDENT LPAREN arguments_opt RPAREN SEMICOLON
      {
        $$ = new CallStatement($2, $4);
        free($2);
      }
      ;

arguments_opt:
      /* empty */
      {
          $$ = new ExpressionList();
      }
    | arguments
      {
          $$ = $1;
      }
    ;

arguments:
      expression
      {
          $$ = new ExpressionList();
          $$->push_back($1);
      }
    | arguments COMMA expression
      {
          $$ = $1;
          $$->push_back($3);
      }
    ;

const_opt:

      {
          $$ = false;
      }
    | CONST
      {
          $$ = true;
      }
    ;

type:
      SIGNED
      {
          $$ = ValueType::Signed;
      }
    | UNSIGNED
      {
          $$ = ValueType::Unsigned;
      }
    ;
// <-
init_opt:

      {
          $$ = nullptr;
      }
    | ASSIGN expression
      {
          $$ = $2;
      }
    ;

expression:
      INT_LITERAL
      {
            $$ = new LiteralExpression(Value::makeSigned($1));
      }
    | UINT_LITERAL
      {
            $$ = new LiteralExpression(Value::makeUnsigned($1));
      }
    | IDENT
      {
            $$ = new VariableExpression($1);
            free($1);
      }
    | expression PLUS expression
      {
            $$ = new BinaryExpression(BinaryOperator::Add, $1, $3);
      }
    | expression MINUS expression
      {
            $$ = new BinaryExpression(BinaryOperator::Subtract, $1, $3);
      }
    | expression STAR expression
      {
            $$ = new BinaryExpression(BinaryOperator::Multiply, $1, $3);
      }
    | expression SLASH expression
      {
            $$ = new BinaryExpression(BinaryOperator::Divide, $1, $3);
      }
    | expression PERCENT expression
      {
            $$ = new BinaryExpression(BinaryOperator::Modulo, $1, $3);
      }
    | MINUS expression %prec UMINUS
      {
            $$ = new UnaryMinusExpression($2);
      }
    | LPAREN expression RPAREN
      {
            $$ = $2;
      }
    | expression LESS expression
      {
            $$ = new BinaryExpression(BinaryOperator::Less, $1, $3);
      }
    | expression GREATER expression
      {
            $$ = new BinaryExpression(BinaryOperator::Greater, $1, $3);
      }
    | expression EQUAL expression
      {
            $$ = new BinaryExpression(BinaryOperator::Equal, $1, $3);
      }
    ;

robot_command:
      TOP
      {
          $$ = CommandKind::Top;
      }
    | BOTTOM
      {
          $$ = CommandKind::Bottom;
      }
    | LEFT
      {
          $$ = CommandKind::Left;
      }
    | RIGHT
      {
          $$ = CommandKind::Right;
      }
    ;

%%

void yyerror(const char* message) {
    std::cerr << "Parser error at line " << yylineno << ": " << message << std::endl;
}