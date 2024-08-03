
#include "../include/csl.h"
/*
  femtoLisp

  a minimal interpreter for a minimal lisp dialect

  this lisp dialect uses lexical scope and self-evaluating lambda.
  it supports 30-bit integers, symbols, conses, and full macros.
  it is case-sensitive.
  it features a simple compacting copying garbage collector.
  it uses a Scheme-style evaluation rule where any expression may appear in
    head position as long as it evaluates to a function.
  it uses Scheme-style varargs (dotted formal argument lists)
  lambdas can have only 1 body expression; use (progn ...) for multiple
    expressions. this is due to the closure representation
    (lambda args body . env)

  by Jeff Bezanson
  Public Domain
 */

// read -----------------------------------------------------------------------

enum
{
    TOK_NONE, TOK_OPEN, TOK_CLOSE, TOK_DOT, TOK_QUOTE, TOK_SYM, TOK_NUM
} ;

static u_int32_t toktype = TOK_NONE ;
static value_t tokval ;
static char *buf ;

int64
fl_getChar ( )
{
    if ( cli ) lic = _Lexer_NextChar ( _Lexer_ ) ;
    else lic = fgetc ( f ) ;
    return lic ;
}

void
fl_ungetChar ( int c )
{
    if ( cli ) ReadLine_UnGetChar ( _ReadLiner_ ) ;
    else ungetc ( c, f ) ;
}

static int
symchar ( char c )
{
    static char *special = "()';\\|" ;
    return (! isspace ( c ) && ! strchr ( special, c ) ) ;
}

static void
take ( void )
{
    toktype = TOK_NONE ;
}

static void
accumchar ( char c, int *pi )
{
    if ( ! cli )
    {
        if ( c == '\b' ) ( *pi ) -- ;
        else buf[( *pi ) ++] = c ;
        if ( *pi >= ( int ) ( BUFFER_SIZE - 1 ) ) //sizeof (buf ) - 1 ) )
            lerror ( "read: error: token too long\n" ) ;
    }
    else
    {
        if ( c == '\b' ) ( *pi ) --, Lexer_UnAppendCharacter ( _Lexer_ ) ;
        else ( *pi ) ++, Lexer_AppendToTokenBuffer ( _Lexer_, c ) ;
    }
}

static char
nextchar ( )
{
    char c ;
    int ch ;

    do
    {
        ch = fl_getChar ( ) ;
        if ( ( ch == EOF ) || ( ch == 0 ) )
            return 0 ;
        c = ( char ) ch ;
        if ( c == ';' )
        {
            // single-line comment
            do
            {
                ch = fl_getChar ( ) ;
                if ( ( ch == EOF ) || ( ch == 0 ) )
                    return 0 ;
            }
            while ( ( char ) ch != '\n' ) ;
            c = ( char ) ch ;
        }
        if ( c == '\b' ) ReadLine_UnGetChar ( _ReadLiner_ ) ;
        if ( ( ( c == '\r' ) || ( c == '\n' ) ) && ( cli ) )
            printf ( "\nflt> " ), fflush ( stdout ) ;
        //else if ( isspace ( c ) ) FL_PutChar ( c ) ;
    }
    while ( isspace ( c ) ) ;
    return c ;
}
// return: 1 for dot token, 0 for symbol

static u_int32_t
peek ( )
{
    char c, *end ;
    number_t x ;
    if ( toktype != TOK_NONE ) return toktype ;
    c = nextchar ( ) ;
    if ( ( c == EOF ) || ( c == 0 ) ) return TOK_NONE ;
    if ( c == '(' ) toktype = TOK_OPEN ;
    else if ( c == ')' ) toktype = TOK_CLOSE ;
    else if ( c == '\'' ) toktype = TOK_QUOTE ;
    else if ( isdigit ( c ) || c == '-' || c == '+' )
    {
        fl_read_token ( c ) ;
        x = strtol ( buf, &end, 0 ) ;
        if ( *end != '\0' )
        {
            toktype = TOK_SYM ;
            tokval = symbol ( buf ) ;
        }
        else
        {
            toktype = TOK_NUM ;
            tokval = number ( x ) ;
        }
    }
    else
    {
        if ( fl_read_token ( c ) ) toktype = TOK_DOT ;
        else
        {
            toktype = TOK_SYM ;
            tokval = symbol ( buf ) ;
        }
    }
    return toktype ;
}

int
fl_read_token ( char c )
{
    Lexer * lexer = _Lexer_ ;
    int inChar, escaped = 0, dot = ( c == '.' ), totread = 0, i = 0 ;
    lexer->TokenWriteIndex = 0 ;
    fl_ungetChar ( c ) ;
    while ( 1 )
    {
        inChar = fl_getChar ( ) ;
        totread ++ ;
        if ( ( inChar == EOF ) || ( inChar == 0 ) ) break ;
        c = ( char ) inChar ;
        if ( c == '|' ) escaped = ! escaped ;
        else if ( c == '\\' )
        {
            inChar = fl_getChar ( ) ;
            if ( ( inChar == EOF ) || ( inChar == 0 ) ) break ;
            accumchar ( ( char ) inChar, &i ) ;
        }
        else if ( ! escaped && ! symchar ( c ) )
        {
            fl_ungetChar ( c ) ;
            break ;
        }
        else accumchar ( c, &i ) ;
    }
    buf [ i ++ ] = '\0' ;
    return (dot && ( totread == 2 ) ) ;
}

// build a list of conses. this is complicated by the fact that all conses
// can move whenever a new cons is allocated. we have to refer to every cons
// through a handle to a relocatable pointer (i.e. a pointer on the stack).

static void
read_list ( value_t *pval )
{
    value_t c, *pc ;
    u_int32_t t ;

    FL_PUSH ( FL_NIL ) ;
    pc = & FL_Stack[SP - 1] ; // to keep track of current cons cell
    t = peek ( ) ;
    while ( t != TOK_CLOSE )
    {
        //if ( feof ( f ) )   lerror ( "read: error: unexpected end of input\n" ) ;
        c = mk_cons ( ) ;
        car_ ( c ) = cdr_ ( c ) = FL_NIL ;
        if ( iscons ( *pc ) )
            cdr_ ( *pc ) = c ;
        else
            *pval = c ;
        *pc = c ;
        c = read_sexpr ( ) ; // must be on separate lines due to undefined
        car_ ( *pc ) = c ; // evaluation order

        t = peek ( ) ;
        if ( t == TOK_DOT )
        {
            take ( ) ;
            c = read_sexpr ( ) ;
            cdr_ ( *pc ) = c ;
            t = peek ( ) ;
            //if ( feof ( f ) )                lerror ( "read: error: unexpected end of input\n" ) ;
            if ( t != TOK_CLOSE )
                lerror ( "read: error: expected ')'\n" ) ;
        }
    }
    take ( ) ;
    FL_POP ( ) ;
}

value_t
read_sexpr ( )
{
    value_t v ;

    switch ( peek ( ) )
    {
        case TOK_CLOSE:
            take ( ) ;
            lerror ( "read: error: unexpected ')'\n" ) ;
        case TOK_DOT:
            take ( ) ;
            lerror ( "read: error: unexpected '.'\n" ) ;
        case TOK_SYM:
        case TOK_NUM:
            take ( ) ;
            return tokval ;
        case TOK_QUOTE:
            take ( ) ;
            v = read_sexpr ( ) ;
            FL_PUSH ( v ) ;
            v = cons_ ( &FL_QUOTE, cons ( &FL_Stack[SP - 1], &FL_NIL ) ) ;
            FL_POPN ( 2 ) ;
            return v ;
        case TOK_OPEN:
            take ( ) ;
            FL_PUSH ( FL_NIL ) ;
            read_list ( &FL_Stack[SP - 1] ) ;
            return FL_POP ( ) ;
    }
    return FL_NIL ;
}

