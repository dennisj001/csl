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
#include "../include/csl.h"

value_t FL_NIL, T, FL_LAMBDA, FL_MACRO, FL_LABEL, FL_QUOTE ;
//value_t lv ; // last value
FILE *f ; // input file
Boolean cli = false ;
//int64 lic ; // last input char 

// error utilities ------------------------------------------------------------

jmp_buf toplevel ;

void
lerror ( char *format, ... )
{
    va_list args ;
    va_start ( args, format ) ;
    vfprintf ( stderr, format, args ) ;
    va_end ( args ) ;
    longjmp ( toplevel, 1 ) ;
}

void
type_error ( char *fname, char *expected, value_t got )
{
    fprintf ( stderr, "%s: error: expected %s, got ", fname, expected ) ;
    print ( stderr, got ) ;
    lerror ( "\n" ) ;
}

// safe cast operators --------------------------------------------------------

#define SAFECAST_OP(type,ctype,cnvt)                                          \
ctype to##type(value_t v, char *fname)                                        \
{                                                                             \
    if (is##type(v))                                                          \
        return (ctype)cnvt(v);                                                \
    type_error(fname, #type, v);                                              \
    return (ctype)0;                                                          \
}
SAFECAST_OP ( cons, cons_t*, ptr )
SAFECAST_OP ( symbol, symbol_t*, ptr )
SAFECAST_OP ( number, number_t, numval )

// symbol table ---------------------------------------------------------------

static symbol_t *symtab = NULL ;

static symbol_t *
mk_symbol ( char *str )
{
    symbol_t *sym ;

    sym = ( symbol_t* ) malloc ( sizeof (symbol_t ) + strlen ( str ) ) ;
    sym->left = sym->right = NULL ;
    sym->constant = sym->binding = UNBOUND ;
    strcpy ( &sym->name[0], str ) ;
    return sym ;
}

static symbol_t **
symtab_lookup ( symbol_t **ptree, char *str )
{
    int x ;

    while ( *ptree != NULL )
    {
        x = strcmp ( str, ( *ptree )->name ) ;
        if ( x == 0 )
            return ptree ;
        if ( x < 0 )
            ptree = & ( *ptree )->left ;
        else
            ptree = & ( *ptree )->right ;
    }
    return ptree ;
}

value_t
symbol ( char *str )
{
    symbol_t **pnode ;

    pnode = symtab_lookup ( &symtab, str ) ;
    if ( *pnode == NULL )
        *pnode = mk_symbol ( str ) ;
    return tagptr ( *pnode, TAG_SYM ) ;
}

// initialization -------------------------------------------------------------

static unsigned char *fromspace ;
static unsigned char *tospace ;
static unsigned char *curheap ;
static unsigned char *lim ;
static u_int32_t heapsize = 64 * 1024 ;

void
lisp_init ( void )
{
    int i ;

    fromspace = malloc ( heapsize ) ;
    tospace = malloc ( heapsize ) ;
    curheap = fromspace ;
    lim = curheap + heapsize - sizeof (cons_t ) ;

    FL_NIL = symbol ( "nil" ) ;
    setc ( FL_NIL, FL_NIL ) ;
    T = symbol ( "t" ) ;
    setc ( T, T ) ;
    FL_LAMBDA = symbol ( "lambda" ) ;
    FL_MACRO = symbol ( "macro" ) ;
    //    FL_LABEL = symbol ( "label" ) ;
    FL_QUOTE = symbol ( "quote" ) ;
    for ( i = 0 ; i < ( int ) N_BUILTINS ; i ++ )
        setc ( symbol ( builtin_names[i] ), builtin ( i ) ) ;
    setc ( symbol ( "princ" ), builtin ( F_PRINT ) ) ;
}

// conses ---------------------------------------------------------------------

void gc ( void ) ;

static value_t
mk_cons ( void )
{
    cons_t *c ;

    if ( curheap > lim )
        gc ( ) ;
    c = ( cons_t* ) curheap ;
    curheap += sizeof (cons_t ) ;
    return tagptr ( c, TAG_CONS ) ;
}

static value_t
cons_ ( value_t *pcar, value_t *pcdr )
{
    value_t c = mk_cons ( ) ;
    car_ ( c ) = * pcar ;
    cdr_ ( c ) = * pcdr ;
    return c ;
}

value_t *
cons ( value_t *pcar, value_t *pcdr )
{
    value_t c = mk_cons ( ) ;
    car_ ( c ) = * pcar ;
    cdr_ ( c ) = * pcdr ;
    FL_PUSH ( c ) ;
    return &FL_Stack[SP - 1] ;
}

// collector ------------------------------------------------------------------

static value_t
relocate ( value_t v )
{
    value_t a, d, nc ;

    if ( ! iscons ( v ) )
        return v ;
    if ( car_ ( v ) == UNBOUND )
        return cdr_ ( v ) ;
    nc = mk_cons ( ) ;
    a = car_ ( v ) ;
    d = cdr_ ( v ) ;
    car_ ( v ) = UNBOUND ;
    cdr_ ( v ) = nc ;
    car_ ( nc ) = relocate ( a ) ;
    cdr_ ( nc ) = relocate ( d ) ;
    return nc ;
}

static void
trace_globals ( symbol_t *root )
{
    while ( root != NULL )
    {
        root->binding = relocate ( root->binding ) ;
        trace_globals ( root->left ) ;
        root = root->right ;
    }
}

void
gc ( void )
{
    static int grew = 0 ;
    unsigned char *temp ;
    u_int32_t i ;

    curheap = tospace ;
    lim = curheap + heapsize - sizeof (cons_t ) ;

    for ( i = 0 ; i < SP ; i ++ )
        FL_Stack[i] = relocate ( FL_Stack[i] ) ;
    trace_globals ( symtab ) ;
#ifdef VERBOSEGC
    printf ( "gc found %d/%d live conses\n", ( curheap - tospace ) / 8, heapsize / 8 ) ;
#endif
    temp = tospace ;
    tospace = fromspace ;
    fromspace = temp ;

    // if we're using > 80% of the space, resize tospace so we have
    // more space to fill next time. if we grew tospace last time,
    // grow the other half of the heap this time to catch up.
    if ( grew || ( ( lim - curheap ) < ( int ) ( heapsize / 5 ) ) )
    {
        temp = realloc ( tospace, grew ? heapsize : heapsize * 2 ) ;
        if ( temp == NULL )
            lerror ( "out of memory\n" ) ;
        tospace = temp ;
        if ( ! grew )
            heapsize *= 2 ;
        grew = ! grew ;
    }
    if ( curheap > lim ) // all data was live
        gc ( ) ;
}

// read -----------------------------------------------------------------------

#include "fltlread.c"

// print ----------------------------------------------------------------------

void
print ( FILE *f, value_t v )
{
    value_t cd ;

    switch ( tag ( v ) )
    {
        case TAG_NUM: fprintf ( f, "%ld", numval ( v ) ) ;
            break ;
        case TAG_SYM: fprintf ( f, "%s", ( ( symbol_t* ) ptr ( v ) )->name ) ;
            break ;
        case TAG_BUILTIN: fprintf ( f, "#<builtin %s>",
                builtin_names[intval ( v )] ) ;
            break ;
        case TAG_CONS:
            fprintf ( f, "(" ) ;
            while ( 1 )
            {
                print ( f, car_ ( v ) ) ;
                cd = cdr_ ( v ) ;
                if ( ! iscons ( cd ) )
                {
                    if ( cd != FL_NIL )
                    {
                        fprintf ( f, " . " ) ;
                        print ( f, cd ) ;
                    }
                    fprintf ( f, ")" ) ;
                    break ;
                }
                fprintf ( f, " " ) ;
                v = cd ;
            }
            break ;
    }
}

byte *
sprint ( value_t v )
{
    byte * b = Buffer_New_pbyte ( BUFFER_SIZE ) ;

    value_t cd ;

    switch ( tag ( v ) )
    {
        case TAG_NUM: sprintf ( b, "%ld", numval ( v ) ) ;
            break ;
        case TAG_SYM: sprintf ( b, "%s", ( ( symbol_t* ) ptr ( v ) )->name ) ;
            break ;
        case TAG_BUILTIN: sprintf ( b, "#<builtin %s>",
                builtin_names[intval ( v )] ) ;
            break ;
        case TAG_CONS:
            sprintf ( b, "(" ) ;
            while ( 1 )
            {
                sprint ( car_ ( v ) ) ;
                cd = cdr_ ( v ) ;
                if ( ! iscons ( cd ) )
                {
                    if ( cd != FL_NIL )
                    {
                        sprintf ( b, " . " ) ;
                        sprint ( cd ) ;
                    }
                    sprintf ( b, ")" ) ;
                    break ;
                }
                sprintf ( b, " " ) ;
                v = cd ;
            }
            break ;
    }
    return b ;
}

// eval ----------------------------------------------------------------------- 

static inline void
argcount ( char *fname, int nargs, int c )
{
    if ( nargs != c )
        lerror ( "%s: error: too %s arguments\n", fname, nargs < c ? "few" : "many" ) ;
}

#define eval(e, env) ((tag(e)<0x2) ? (e) : eval_sexpr((e),env))
#define tail_eval(xpr, env) do { SP = saveSP;  \
    if (tag(xpr)<0x2) { return (xpr); } \
    else { e=(xpr); *penv=(env); goto eval_top; } } while (0)

value_t
eval_sexpr ( value_t e, value_t *penv )
{
    value_t f, v, bind, headsym, asym, labl = 0, *pv, *argsyms, *body, *lenv ;
    value_t *rest ;
    cons_t *c ;
    symbol_t *sym ;
    u_int32_t saveSP ;
    int i, nargs, noeval = 0 ;
    number_t s, n ;

eval_top:
    if ( issymbol ( e ) )
    {
        sym = ( symbol_t* ) ptr ( e ) ;
        if ( sym->constant != UNBOUND ) return sym->constant ;
        v = * penv ;
        while ( iscons ( v ) )
        {
            bind = car_ ( v ) ;
            if ( iscons ( bind ) && car_ ( bind ) == e )
                return cdr_ ( bind ) ;
            v = cdr_ ( v ) ;
        }
        if ( ( v = sym->binding ) == UNBOUND )
            lerror ( "eval: error: variable %s has no value\n", sym->name ) ;
        return v ;
    }
    if ( ( unsigned long ) ( char* ) &nargs < ( unsigned long ) stack_bottom || SP >= ( N_STACK - 100 ) )
        lerror ( "eval: error: stack overflow\n" ) ;
    saveSP = SP ;
    FL_PUSH ( e ) ;
    FL_PUSH ( *penv ) ;
    f = eval ( car_ ( e ), penv ) ;
    *penv = FL_Stack[saveSP + 1] ;
    if ( isbuiltin ( f ) )
    {
        // handle builtin function
        if ( ! isspecial ( f ) )
        {
            // evaluate argument list, placing arguments on stack
            v = FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
            while ( iscons ( v ) )
            {
                v = eval ( car_ ( v ), penv ) ;
                *penv = FL_Stack[saveSP + 1] ;
                FL_PUSH ( v ) ;
                v = FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
            }
        }
apply_builtin:
        nargs = SP - saveSP - 2 ;
        switch ( intval ( f ) )
        {
                // special forms
            case F_QUOTE:
                v = cdr_ ( FL_Stack[saveSP] ) ;
                if ( ! iscons ( v ) )
                    lerror ( "quote: error: expected argument\n" ) ;
                v = car_ ( v ) ;
                break ;
            case F_MACRO:
            case F_LAMBDA:
                v = FL_Stack[saveSP] ;
                if ( *penv != FL_NIL )
                {
                    // build a closure (lambda args body . env)
                    v = cdr_ ( v ) ;
                    FL_PUSH ( car ( v ) ) ;
                    argsyms = & FL_Stack[SP - 1] ;
                    FL_PUSH ( car ( cdr_ ( v ) ) ) ;
                    body = & FL_Stack[SP - 1] ;
                    v = cons_ ( intval ( f ) == F_LAMBDA ? &FL_LAMBDA : &FL_MACRO,
                        cons ( argsyms, cons ( body, penv ) ) ) ;
                }
                break ;
            case F_LABEL:
                v = FL_Stack[saveSP] ;
                if ( *penv != FL_NIL )
                {
                    v = cdr_ ( v ) ;
                    FL_PUSH ( car ( v ) ) ; // name
                    pv = & FL_Stack[SP - 1] ;
                    FL_PUSH ( car ( cdr_ ( v ) ) ) ; // function
                    body = & FL_Stack[SP - 1] ;
                    *body = eval ( *body, penv ) ; // evaluate lambda
                    v = cons_ ( &FL_LABEL, cons ( pv, cons ( body, &FL_NIL ) ) ) ;
                }
                break ;
            case F_IF:
                v = car ( cdr_ ( FL_Stack[saveSP] ) ) ;
                if ( eval ( v, penv ) != FL_NIL )
                    v = car ( cdr_ ( cdr_ ( FL_Stack[saveSP] ) ) ) ;
                else
                    v = car ( cdr ( cdr_ ( cdr_ ( FL_Stack[saveSP] ) ) ) ) ;
                tail_eval ( v, FL_Stack[saveSP + 1] ) ;
                break ;
            case F_COND:
                FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
                pv = & FL_Stack[saveSP] ;
                v = FL_NIL ;
                while ( iscons ( *pv ) )
                {
                    c = tocons ( car_ ( *pv ), "cond" ) ;
                    v = eval ( c->car, penv ) ;
                    *penv = FL_Stack[saveSP + 1] ;
                    if ( v != FL_NIL )
                    {
                        *pv = cdr_ ( car_ ( *pv ) ) ;
                        // evaluate body forms
                        if ( iscons ( *pv ) )
                        {
                            while ( iscons ( cdr_ ( *pv ) ) )
                            {
                                v = eval ( car_ ( *pv ), penv ) ;
                                *penv = FL_Stack[saveSP + 1] ;
                                *pv = cdr_ ( *pv ) ;
                            }
                            tail_eval ( car_ ( *pv ), *penv ) ;
                        }
                        break ;
                    }
                    *pv = cdr_ ( *pv ) ;
                }
                break ;
            case F_AND:
                FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
                pv = & FL_Stack[saveSP] ;
                v = T ;
                if ( iscons ( *pv ) )
                {
                    while ( iscons ( cdr_ ( *pv ) ) )
                    {
                        if ( ( v = eval ( car_ ( *pv ), penv ) ) == FL_NIL )
                        {
                            SP = saveSP ;
                            return FL_NIL ;
                        }
                        *penv = FL_Stack[saveSP + 1] ;
                        *pv = cdr_ ( *pv ) ;
                    }
                    tail_eval ( car_ ( *pv ), *penv ) ;
                }
                break ;
            case F_OR:
                FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
                pv = & FL_Stack[saveSP] ;
                v = FL_NIL ;
                if ( iscons ( *pv ) )
                {
                    while ( iscons ( cdr_ ( *pv ) ) )
                    {
                        if ( ( v = eval ( car_ ( *pv ), penv ) ) != FL_NIL )
                        {
                            SP = saveSP ;
                            return v ;
                        }
                        *penv = FL_Stack[saveSP + 1] ;
                        *pv = cdr_ ( *pv ) ;
                    }
                    tail_eval ( car_ ( *pv ), *penv ) ;
                }
                break ;
            case F_WHILE:
                FL_PUSH ( cdr ( cdr_ ( FL_Stack[saveSP] ) ) ) ;
                body = & FL_Stack[SP - 1] ;
                FL_PUSH ( *body ) ;
                FL_Stack[saveSP] = car_ ( cdr_ ( FL_Stack[saveSP] ) ) ;
                value_t *cond = & FL_Stack[saveSP] ;
                FL_PUSH ( FL_NIL ) ;
                pv = & FL_Stack[SP - 1] ;
                while ( eval ( *cond, penv ) != FL_NIL )
                {
                    *penv = FL_Stack[saveSP + 1] ;
                    *body = FL_Stack[SP - 2] ;
                    while ( iscons ( *body ) )
                    {
                        *pv = eval ( car_ ( *body ), penv ) ;
                        *penv = FL_Stack[saveSP + 1] ;
                        *body = cdr_ ( *body ) ;
                    }
                }
                v = * pv ;
                break ;
            case F_PROGN:
                // return last arg
                FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
                pv = & FL_Stack[saveSP] ;
                v = FL_NIL ;
                if ( iscons ( *pv ) )
                {
                    while ( iscons ( cdr_ ( *pv ) ) )
                    {
                        v = eval ( car_ ( *pv ), penv ) ;
                        *penv = FL_Stack[saveSP + 1] ;
                        *pv = cdr_ ( *pv ) ;
                    }
                    tail_eval ( car_ ( *pv ), *penv ) ;
                }
                break ;

                // ordinary functions
            case F_SET:
                argcount ( "set", nargs, 2 ) ;
                e = FL_Stack[SP - 2] ;
                v = * penv ;
                while ( iscons ( v ) )
                {
                    bind = car_ ( v ) ;
                    if ( iscons ( bind ) && car_ ( bind ) == e )
                    {
                        cdr_ ( bind ) = ( v = FL_Stack[SP - 1] ) ;
                        SP = saveSP ;
                        return v ;
                    }
                    v = cdr_ ( v ) ;
                }
                tosymbol ( e, "set" )->binding = ( v = FL_Stack[SP - 1] ) ;
                break ;
            case F_BOUNDP:
                argcount ( "boundp", nargs, 1 ) ;
                sym = tosymbol ( FL_Stack[SP - 1], "boundp" ) ;
                if ( sym->binding == UNBOUND && sym->constant == UNBOUND )
                    v = FL_NIL ;
                else
                    v = T ;
                break ;
            case F_EQ:
                argcount ( "eq", nargs, 2 ) ;
                v = ( ( FL_Stack[SP - 2] == FL_Stack[SP - 1] ) ? T : FL_NIL ) ;
                break ;
            case F_EXIT:
                return - 1 ;
                break ;
            case F_RETURN:
                DataStack_Push ( ( int64 ) sprint ( lv ) ) ;
                return - 1 ;
                break ;
            case F_CONS:
                argcount ( "cons", nargs, 2 ) ;
                v = mk_cons ( ) ;
                car_ ( v ) = FL_Stack[SP - 2] ;
                cdr_ ( v ) = FL_Stack[SP - 1] ;
                break ;
            case F_CAR:
                argcount ( "car", nargs, 1 ) ;
                v = car ( FL_Stack[SP - 1] ) ;
                break ;
            case F_CDR:
                argcount ( "cdr", nargs, 1 ) ;
                v = cdr ( FL_Stack[SP - 1] ) ;
                break ;
            case F_RPLACA:
                argcount ( "rplaca", nargs, 2 ) ;
                car ( v = FL_Stack[SP - 2] ) = FL_Stack[SP - 1] ;
                break ;
            case F_RPLACD:
                argcount ( "rplacd", nargs, 2 ) ;
                cdr ( v = FL_Stack[SP - 2] ) = FL_Stack[SP - 1] ;
                break ;
            case F_ATOM:
                argcount ( "atom", nargs, 1 ) ;
                v = ( ( ! iscons ( FL_Stack[SP - 1] ) ) ? T : FL_NIL ) ;
                break ;
            case F_SYMBOLP:
                argcount ( "symbolp", nargs, 1 ) ;
                v = ( ( issymbol ( FL_Stack[SP - 1] ) ) ? T : FL_NIL ) ;
                break ;
            case F_NUMBERP:
                argcount ( "numberp", nargs, 1 ) ;
                v = ( ( isnumber ( FL_Stack[SP - 1] ) ) ? T : FL_NIL ) ;
                break ;
            case F_ADD:
                s = 0 ;
                for ( i = saveSP + 2 ; i < ( int ) SP ; i ++ )
                {
                    n = tonumber ( FL_Stack[i], "+" ) ;
                    s += n ;
                }
                v = number ( s ) ;
                break ;
            case F_SUB:
                if ( nargs < 1 )
                    lerror ( "-: error: too few arguments\n" ) ;
                i = saveSP + 2 ;
                s = ( nargs == 1 ) ? 0 : tonumber ( FL_Stack[i ++], "-" ) ;
                for ( ; i < ( int ) SP ; i ++ )
                {
                    n = tonumber ( FL_Stack[i], "-" ) ;
                    s -= n ;
                }
                v = number ( s ) ;
                break ;
            case F_MUL:
                s = 1 ;
                for ( i = saveSP + 2 ; i < ( int ) SP ; i ++ )
                {
                    n = tonumber ( FL_Stack[i], "*" ) ;
                    s *= n ;
                }
                v = number ( s ) ;
                break ;
            case F_DIV:
                if ( nargs < 1 )
                    lerror ( "/: error: too few arguments\n" ) ;
                i = saveSP + 2 ;
                s = ( nargs == 1 ) ? 1 : tonumber ( FL_Stack[i ++], "/" ) ;
                for ( ; i < ( int ) SP ; i ++ )
                {
                    n = tonumber ( FL_Stack[i], "/" ) ;
                    if ( n == 0 )
                        lerror ( "/: error: division by zero\n" ) ;
                    s /= n ;
                }
                v = number ( s ) ;
                break ;
            case F_LT:
                argcount ( "<", nargs, 2 ) ;
                if ( tonumber ( FL_Stack[SP - 2], "<" ) < tonumber ( FL_Stack[SP - 1], "<" ) )
                    v = T ;
                else
                    v = FL_NIL ;
                break ;
            case F_NOT:
                argcount ( "not", nargs, 1 ) ;
                v = ( ( FL_Stack[SP - 1] == FL_NIL ) ? T : FL_NIL ) ;
                break ;
            case F_EVAL:
                argcount ( "eval", nargs, 1 ) ;
                v = FL_Stack[SP - 1] ;
                tail_eval ( v, FL_NIL ) ;
                break ;
            case F_PRINT:
                for ( i = saveSP + 2 ; i < ( int ) SP ; i ++ )
                    print ( stdout, v = FL_Stack[i] ) ;
                break ;
            case F_READ:
                argcount ( "read", nargs, 0 ) ;
                v = read_sexpr ( ) ;
                break ;
            case F_LOAD:
                argcount ( "load", nargs, 1 ) ;
                cli = false ; //lf = true ;
                v = load_file ( tosymbol ( FL_Stack[SP - 1], "load" )->name ) ;
                cli = true ;
                break ;
            case F_PROG1:
                // return first arg
                if ( nargs < 1 )
                    lerror ( "prog1: error: too few arguments\n" ) ;
                v = FL_Stack[saveSP + 2] ;
                break ;
            case F_APPLY:
                argcount ( "apply", nargs, 2 ) ;
                v = FL_Stack[saveSP] = FL_Stack[SP - 1] ; // second arg is new arglist
                f = FL_Stack[SP - 2] ; // first arg is new function
                FL_POPN ( 2 ) ; // pop apply's args
                if ( isbuiltin ( f ) )
                {
                    if ( isspecial ( f ) )
                        lerror ( "apply: error: cannot apply special operator "
                        "%s\n", builtin_names[intval ( f )] ) ;
                    // unpack arglist onto the stack
                    while ( iscons ( v ) )
                    {
                        FL_PUSH ( car_ ( v ) ) ;
                        v = cdr_ ( v ) ;
                    }
                    goto apply_builtin ;
                }
                noeval = 1 ;
                goto apply_lambda ;
        }
        SP = saveSP ;
        return v ;
    }
    else
    {
        v = FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
    }
apply_lambda:
    if ( iscons ( f ) )
    {
        headsym = car_ ( f ) ;
        if ( headsym == FL_LABEL )
        {
            // (label name (lambda ...)) behaves the same as the lambda
            // alone, except with name bound to the whole label expression
            labl = f ;
            f = car ( cdr ( cdr_ ( labl ) ) ) ;
            headsym = car ( f ) ;
        }
        // apply lambda or macro expression
        FL_PUSH ( cdr ( cdr ( cdr_ ( f ) ) ) ) ;
        lenv = & FL_Stack[SP - 1] ;
        FL_PUSH ( car_ ( cdr_ ( f ) ) ) ;
        argsyms = & FL_Stack[SP - 1] ;
        FL_PUSH ( car_ ( cdr_ ( cdr_ ( f ) ) ) ) ;
        body = & FL_Stack[SP - 1] ;
        if ( labl )
        {
            // add label binding to environment
            FL_PUSH ( labl ) ;
            FL_PUSH ( car_ ( cdr_ ( labl ) ) ) ;
            *lenv = cons_ ( cons ( &FL_Stack[SP - 1], &FL_Stack[SP - 2] ), lenv ) ;
            FL_POPN ( 3 ) ;
            v = FL_Stack[saveSP] ; // refetch arglist
        }
        if ( headsym == FL_MACRO )
            noeval = 1 ;
        else if ( headsym != FL_LAMBDA )
            lerror ( "apply: error: head must be lambda, macro, or label\n" ) ;
        // build a calling environment for the lambda
        // the environment is the argument binds on top of the captured
        // environment
        while ( iscons ( v ) )
        {
            // bind args
            if ( ! iscons ( *argsyms ) )
            {
                if ( *argsyms == FL_NIL )
                    lerror ( "apply: error: too many arguments\n" ) ;
                break ;
            }
            asym = car_ ( *argsyms ) ;
            if ( ! issymbol ( asym ) )
                lerror ( "apply: error: formal argument not a symbol\n" ) ;
            v = car_ ( v ) ;
            if ( ! noeval )
            {
                v = eval ( v, penv ) ;
                *penv = FL_Stack[saveSP + 1] ;
            }
            FL_PUSH ( v ) ;
            *lenv = cons_ ( cons ( &asym, &FL_Stack[SP - 1] ), lenv ) ;
            FL_POPN ( 2 ) ;
            *argsyms = cdr_ ( *argsyms ) ;
            v = FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
        }
        if ( *argsyms != FL_NIL )
        {
            if ( issymbol ( *argsyms ) )
            {
                if ( noeval )
                {
                    *lenv = cons_ ( cons ( argsyms, &FL_Stack[saveSP] ), lenv ) ;
                }
                else
                {
                    FL_PUSH ( FL_NIL ) ;
                    FL_PUSH ( FL_NIL ) ;
                    rest = & FL_Stack[SP - 1] ;
                    // build list of rest arguments
                    // we have to build it forwards, which is tricky
                    while ( iscons ( v ) )
                    {
                        v = eval ( car_ ( v ), penv ) ;
                        *penv = FL_Stack[saveSP + 1] ;
                        FL_PUSH ( v ) ;
                        v = cons_ ( &FL_Stack[SP - 1], &FL_NIL ) ;
                        FL_POP ( ) ;
                        if ( iscons ( *rest ) )
                            cdr_ ( *rest ) = v ;
                        else
                            FL_Stack[SP - 2] = v ;
                        *rest = v ;
                        v = FL_Stack[saveSP] = cdr_ ( FL_Stack[saveSP] ) ;
                    }
                    *lenv = cons_ ( cons ( argsyms, &FL_Stack[SP - 2] ), lenv ) ;
                }
            }
            else if ( iscons ( *argsyms ) )
            {
                lerror ( "apply: error: too few arguments\n" ) ;
            }
        }
        noeval = 0 ;
        // macro: evaluate expansion in the calling environment
        if ( headsym == FL_MACRO )
        {
            SP = saveSP ;
            FL_PUSH ( *lenv ) ;
            lenv = & FL_Stack[SP - 1] ;
            v = eval ( *body, lenv ) ;
            tail_eval ( v, *penv ) ;
        }
        else
        {
            tail_eval ( *body, *lenv ) ;
        }
        // not reached
    }
    type_error ( "apply", "function", f ) ;
    return FL_NIL ;
}

// repl -----------------------------------------------------------------------

static char *infile = NULL ;

value_t
toplevel_eval ( value_t expr )
{
    value_t v ;
    u_int32_t saveSP = SP ;
    FL_PUSH ( FL_NIL ) ;
    v = eval ( expr, &FL_Stack[SP - 1] ) ;
    SP = saveSP ;
    return v ;
}

value_t
load_file ( char *fname )
{
    value_t e, v = FL_NIL ;
    char *lastfile = infile ;
    f = fopen ( fname, "r" ) ;
    infile = fname ;
    if ( f == NULL ) lerror ( "file not found\n" ) ;
    cli = 0 ; // essential logic control
    while ( 1 )
    {
        e = read_sexpr () ;
        if ( feof ( f ) ) break ;
        v = toplevel_eval ( e ) ;
    }
    infile = lastfile ;
    fclose ( f ) ;
    cli = 1 ;
    return v ;
}

int
fl_main ( int argc, char* argv[] )
{
    ReadLiner * rl = _ReadLiner_ ;
    value_t v ;
    cli = false ;
    stack_bottom = ( ( char* ) &v ) - PROCESS_STACK_SIZE ;
    lisp_init ( ) ;
    buf = _Lexer_->TokenBuffer ;
    if ( setjmp ( toplevel ) )
    {
        SP = 0 ;
        fprintf ( stderr, "\n" ) ;
        if ( infile )
        {
            fprintf ( stderr, "error loading file \"%s\"\n", infile ) ;
            infile = NULL ;
        }
        goto repl ;
    }
    load_file ( "/home/dennisj/csl/bin/system.lsp" ) ;
    if ( argc > 1 )
    {
        load_file ( argv[1] ) ;
        return 0 ;
    }
    printf ( "femtoLisp - tiny : type '(exit)' to exit" ) ;
repl:

    cli = true ;
    _ReadLiner_->InputFile = stdin ;
    ReadLine_Init ( rl, _CSL_Key ) ;
    buf = _Lexer_->TokenBuffer ;
    SetState ( _Context_->System0, ADD_READLINE_TO_HISTORY, true ) ;
    byte * snp = rl->NormalPrompt, *sap = rl->AltPrompt ;
    rl->AltPrompt = ( byte* ) "flt< " ;
    rl->NormalPrompt = ( byte* ) "flt> " ;
    while ( 1 )
    {
        if ( ( lic != '\r' ) ) printf ( "\nflt> " ), fflush ( stdout ) ;
        ReadLine_GetLine () ;
        v = read_sexpr ( ) ;
        if ( feof ( stdin ) ) break ;
        v = toplevel_eval ( v ) ;
        lv = v ;
        if ( v == - 1 ) break ;
        print ( stdout, v ) ; 
        set ( symbol ( "that" ), v ) ;
    }
    rl->NormalPrompt = snp ;
    rl->AltPrompt = sap ;
    iPrintf ( "\nfemtolisp - tiny : exiting ... " ) ;
    return 0 ;
}

void
CSL_Flisp ( )
{
    fl_main ( 1, ( char*[] ) { "flisp" } ) ;
}
