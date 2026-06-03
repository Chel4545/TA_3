%option noyywrap
%option yylineno

%{
    #include "parser.hpp"

    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <string>
%}

%%

[ \t\r\n]+              { }

"func"                  { return FUNC; }

"signed"                { return SIGNED; }
"unsigned"              { return UNSIGNED; }
"const"                 { return CONST; }

"testonce"              { return TESTONCE; }
"testrep"               { return TESTREP; }
"call"                  { return CALL; }

"top"                   { return TOP; }
"bottom"                { return BOTTOM; }
"left"                  { return LEFT; }
"right"                 { return RIGHT; }

"xray"                  { return XRAY; }

"<-"                    { return ASSIGN; }

"<"                     { return LESS; }
">"                     { return GREATER; }
"="                     { return EQUAL; }

"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return STAR; }
"/"                     { return SLASH; }
"%"                     { return PERCENT; }

"("                     { return LPAREN; }
")"                     { return RPAREN; }
","                     { return COMMA; }
";"                     { return SEMICOLON; }

[0-9]+[uU]              {
                          std::string text(yytext);
                          text.pop_back();
                          yylval.uint_value = std::strtoull(text.c_str(), nullptr, 10);
                          return UINT_LITERAL;
                        }

[0-9]+                  {
                          yylval.int_value = std::strtoll(yytext, nullptr, 10);
                          return INT_LITERAL;
                        }

[A-Za-z_][A-Za-z0-9_]*  { yylval.str = strdup(yytext);
                          return IDENT; }

.                       {
                        std::fprintf(stderr, "Unknown character '%s' at line %d\n", yytext, yylineno);
                        return yytext[0];
                        }

%%