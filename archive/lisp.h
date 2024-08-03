#if 1 //def __LP64__
typedef u_int64_t value_t ;
typedef int64_t number_t ;
#else
typedef u_int32_t value_t ;
typedef int32_t number_t ;
#endif

typedef struct
{
    value_t car ;
    value_t cdr ;
} cons_t ;

typedef struct _symbol_t
{
    value_t binding ; // global value binding
    value_t constant ; // constant binding (used only for builtins)
    struct _symbol_t *left ;
    struct _symbol_t *right ;
    char name[1] ;
} symbol_t ;

#define TAG_NUM      0x0
#define TAG_BUILTIN  0x1
#define TAG_SYM      0x2
#define TAG_CONS     0x3
#define UNBOUND      ((value_t)TAG_SYM) // an invalid symbol pointer
#define tag(x) ((x)&0x3)
#define ptr(x) ((void*)((x)&(~(value_t)0x3)))
#define tagptr(p,t) (((value_t)(p)) | (t))
#define number(x) ((value_t)((x)<<2))
#define numval(x)  (((number_t)(x))>>2)
#define intval(x)  (((int)(x))>>2)
#define builtin(n) tagptr((((int)n)<<2), TAG_BUILTIN)
#define iscons(x)    (tag(x) == TAG_CONS)
#define issymbol(x)  (tag(x) == TAG_SYM)
#define isnumber(x)  (tag(x) == TAG_NUM)
#define isbuiltin(x) (tag(x) == TAG_BUILTIN)
// functions ending in _ are unsafe, faster versions
#define car_(v) (((cons_t*)ptr(v))->car)
#define cdr_(v) (((cons_t*)ptr(v))->cdr)
#define car(v)  (tocons((v),"car")->car)
#define cdr(v)  (tocons((v),"cdr")->cdr)
#define set(s, v)  (((symbol_t*)ptr(s))->binding = (v))
#define setc(s, v) (((symbol_t*)ptr(s))->constant = (v))

enum
{
    // special forms
    F_QUOTE = 0, F_COND, F_IF, F_AND, F_OR, F_WHILE, F_LAMBDA, F_MACRO, F_LABEL,
    F_PROGN,
    // functions
    F_EQ, F_ATOM, F_CONS, F_CAR, F_CDR, F_READ, F_EVAL, F_PRINT, F_SET, F_NOT,
    F_LOAD, F_SYMBOLP, F_NUMBERP, F_ADD, F_SUB, F_MUL, F_DIV, F_LT, F_PROG1,
    F_APPLY, F_RPLACA, F_RPLACD, F_BOUNDP, F_EXIT, F_RETURN, N_BUILTINS
} ;
#define isspecial(v) (intval(v) <= (int)F_PROGN)

static char *builtin_names[] = { "quote", "cond", "if", "and", "or", "while", "lambda", "macro", "label",
    "progn", "eq", "atom", "cons", "car", "cdr", "read", "eval", "print",
    "set", "not", "load", "symbolp", "numberp", "+", "-", "*", "/", "<",
    "prog1", "apply", "rplaca", "rplacd", "boundp", "exit", "return" } ;

static char *stack_bottom ;
#define PROCESS_STACK_SIZE (2*1024*1024)
#define N_STACK 49152
static value_t FL_Stack [N_STACK] ;
static u_int32_t SP = 0 ;
#define FL_PUSH(v) (FL_Stack[SP++] = (v))
#define FL_POP()   (FL_Stack[--SP])
#define FL_POPN(n) (SP-=(n))

#if 0
value_t read_sexpr ( FILE *f ) ;
void print ( FILE *f, value_t v ) ;
value_t eval_sexpr ( value_t e, value_t *penv ) ;
value_t load_file ( char *fname ) ;
#endif
