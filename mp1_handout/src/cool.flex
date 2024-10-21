/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include "cool_parse.h"
#include "utils.h"
#include "stringtab.h"

#define yylval cool_yylval
#define yylex  cool_yylex

#define MAX_STR_CONST 1025
#define YY_NO_UNPUT

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
extern FILE *fin; /* we read from this file */
#undef YY_INPUT
#define YY_INPUT(buf, result, max_size)                                        \
  if ((result = fread((char *)buf, sizeof(char), max_size, fin)) < 0)          \
    YY_FATAL_ERROR("read() in flex scanner failed");

extern int curr_lineno;

/*
 *  Add Your own definitions here
 */

char buffer[MAX_STR_CONST];
int len = 0;
int comment = 0;
int nested_comment = 0;

bool check(char c) {
    if (len + 1 < MAX_STR_CONST) {
        buffer[len++] = c;
        return true;
    } else {
        return false;
    }
}

%}

%option noyywrap

%x NESTED_COMMENT
%x COMMENT
%x STRING
%x STRING_ERROR
%x ML_STRING
%x ML_STRING_ERROR

/*
 * Define names for regular expressions here.
 */

DIGIT           [0-9]
LETTER          [a-zA-Z]
TYPEID          [A-Z][a-zA-Z0-9_]*
OBJECTID        [a-z][a-zA-Z0-9_]*
DARROW          =>
ASSIGN          <-
LE              <=
WHITESPACE      [ \t\r\v\f]+

%%

 /*
  * Define regular expressions for the tokens of COOL here. Make sure, you
  * handle correctly special cases, like:
  *   - Nested comments
  *   - String constants: They use C like systax and can contain escape
  *     sequences. Escape sequence \c is accepted for all characters c. Except
  *     for \n \t \b \f, the result is c.
  *   - Keywords: They are case-insensitive except for the values true and
  *     false, which must begin with a lower-case letter.
  *   - Multiple-character operators (like <-): The scanner should produce a
  *     single token for every such operator.
  *   - Line counting: You should keep the global variable curr_lineno updated
  *     with the correct line number
  */

"--"            { BEGIN(COMMENT); }
<COMMENT>\n     { curr_lineno++; BEGIN(INITIAL); }
<COMMENT>.      { }

"(*"            { comment = 1; BEGIN(NESTED_COMMENT); nested_comment = 1; }
<NESTED_COMMENT>"(*"   { comment++; }
<NESTED_COMMENT>"*)"   { 
    comment--;

    if (comment < 0) {
        cool_yylval.error_msg = "Unmatched *)";
	    return (ERROR);
    }

    if (comment == 0) {
        nested_comment = 0;
        BEGIN(INITIAL);
    }
}

<NESTED_COMMENT>\n     { curr_lineno++; }
<NESTED_COMMENT>.      { }

<NESTED_COMMENT><<EOF>> {
    cool_yylval.error_msg = "EOF in comment";
    BEGIN(INITIAL);
    return ERROR;
}

"*)" {
    if (!nested_comment) {
        cool_yylval.error_msg = "Unmatched *)";
	    return (ERROR);
    }
}

\"              { len = 0; BEGIN(STRING); } 
\"\"\"          { len = 0; BEGIN(ML_STRING); }    

<STRING>\"      { 
    buffer[len] = '\0';
    cool_yylval.symbol = stringtable.add_string(buffer);
    BEGIN(INITIAL);
    return STR_CONST;
}

<STRING>\0 {
    cool_yylval.error_msg = "String contains null character";
    BEGIN(INITIAL);
    return ERROR;
}

<STRING>\n      {
    cool_yylval.error_msg = "Unterminated string constant";
    curr_lineno++;
    BEGIN(INITIAL);
    return ERROR;
}

<STRING>\\n     { check('\n'); }
<STRING>\\t     { check('\t'); }
<STRING>\\b     { check('\b'); }
<STRING>\\f     { check('\f'); }
<STRING>\\(.)   { check(yytext[1]); }

<STRING>.       {
    if (!check(yytext[0])) {
        cool_yylval.error_msg = "String constant too long";
        BEGIN(STRING_ERROR);
        return ERROR;
    }
}

<STRING><<EOF>> {
    cool_yylval.error_msg = "EOF in string constant";
    BEGIN(INITIAL);
    return ERROR;
}

<STRING_ERROR>\"    { BEGIN(INITIAL); }
<STRING_ERROR>\n    { curr_lineno++; }
<STRING_ERROR>.     { }

<ML_STRING>\"\"\" {
    buffer[len] = '\0';
    cool_yylval.symbol = stringtable.add_string(buffer);
    BEGIN(INITIAL);
    return STR_CONST;
}

<ML_STRING>\0 {
    cool_yylval.error_msg = "String contains null character";
    BEGIN(INITIAL);
    return ERROR;
}

<ML_STRING>\n      { check('\n'); curr_lineno++; }
<ML_STRING>\\n     { check('\n'); }
<ML_STRING>\\t     { check('\t'); }
<ML_STRING>\\b     { check('\b'); }
<ML_STRING>\\f     { check('\f'); }
<ML_STRING>\\(.)   { check(yytext[1]); }

<ML_STRING>.       {
    if (!check(yytext[0])) {
        cool_yylval.error_msg = "String constant too long";
        BEGIN(ML_STRING_ERROR);
        return ERROR;
    }
}

<ML_STRING><<EOF>> {
    cool_yylval.error_msg = "EOF in string constant";
    BEGIN(INITIAL);
    return ERROR;
}

<ML_STRING_ERROR>\"\"\" { BEGIN(INITIAL); }
<ML_STRING_ERROR>\n     { curr_lineno++; }
<ML_STRING_ERROR>.      { }


(?i:class)      { return CLASS; }
(?i:else)       { return ELSE; }
(?i:if)         { return IF; }
(?i:fi)         { return FI; }
(?i:in)         { return IN; }
(?i:inherits)   { return INHERITS; }
(?i:let)        { return LET; }
(?i:loop)       { return LOOP; }
(?i:pool)       { return POOL; }
(?i:then)       { return THEN; }
(?i:while)      { return WHILE; }
(?i:case)       { return CASE; }
(?i:esac)       { return ESAC; }
(?i:of)         { return OF; }
(?i:new)        { return NEW; }
(?i:isvoid)     { return ISVOID; }
(?i:not)        { return NOT; }
t(?i:rue)       { cool_yylval.boolean = true; return BOOL_CONST; }
f(?i:alse)      { cool_yylval.boolean = false; return BOOL_CONST; }

{TYPEID}        { cool_yylval.symbol = idtable.add_string(yytext); return TYPEID; }
{OBJECTID}      { cool_yylval.symbol = idtable.add_string(yytext); return OBJECTID; }
{DIGIT}+        { cool_yylval.symbol = inttable.add_string(yytext); return INT_CONST; }

"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
":"             { return ':'; }
";"             { return ';'; }
","             { return ','; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"~"             { return '~'; }
"<"             { return '<'; }
"="             { return '='; }
"@"             { return '@'; }
"."             { return '.'; }

{DARROW}        { return DARROW; }
{ASSIGN}        { return ASSIGN; }
{LE}            { return LE; }

{WHITESPACE}+   { }
\n              { curr_lineno++; }


.               { 
    cool_yylval.error_msg = yytext;
    return ERROR;
}

%%
